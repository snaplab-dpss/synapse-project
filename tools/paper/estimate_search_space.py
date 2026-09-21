#!/usr/bin/env python3
"""Estimate how many implementations of each NF synapse's search has to choose between.

    ./tools/paper/estimate_search_space.py                      # every NF, one row each
    ./tools/paper/estimate_search_space.py --nfs nat kvs --per-workload
    ./tools/paper/estimate_search_space.py --csv search-space.csv

The number comes out of the searches we already ran. `synapse` counts, for every BDD node, how many
implementations the module factories offered there, and since an implementation is one choice per
node, their product is the size of the space; it reports that as `search_meta.log10_design_space`
in synthesized/<solution>.json (LibSynapse/Search.cpp) and prints it as "Design space" while it
searches. This script reads that field, and falls back to multiplying the per-node counts in
`search_meta.avg_children_per_node`, which is the same calculation, for reports written before
synapse computed it. Every size printed here is a log10 exponent, rounded down -- a 22 means at
least 10^22 implementations -- because the numbers themselves overflow a float.

Each NF was synthesized once per workload (5 churn rates x 7 zipf parameters = 35), and every one
of those runs is a separate search that saw its own numbers, so a NF gets a distribution rather
than a single size. The number to quote for a NF is the MAXIMUM over its workloads: one real
search faced a space that big, which makes it a lower bound on what the NF demands. The minimum
says more about how early that particular search stopped -- nodes it never reached count as 1 --
than about the NF.

Deliberately not reported: the per-node maxima taken across workloads and multiplied together.
That envelope is larger, but its factors come from different searches -- one node's maximum from
one workload, another node's from a different one -- so it describes a space no single search
ever faces, and it is the one aggregation here that is not conservative.

What the per-node counts already include, because they are measured during a real search:
  - the data structure each stateful object is mapped to, with the pruning that follows from it
    (Context::can_impl_ds commits an object to one implementation for the rest of a plan, so a node
    visited afterwards really does offer fewer choices, and that is what gets recorded);
  - the parameters swept inside one module (FCFS cache capacities, the 9 CMS widths of HHTable);
  - BDD reordering: ModuleFactory::implement returns the implementations PLUS their reordered
    variants, and every solution in synthesized/ was generated with reordering enabled.

Where it is deliberately conservative:
  - only Tofino implementations are counted (Search.cpp only accumulates `children` for that
    target), so every controller and x86 alternative is ignored;
  - a node the search never saw, or never saw offering a choice, counts as exactly 1;
  - each per-node count is floored to an integer. Search.cpp only folds a visit into a node's
    running mean when it had more than one successor, which biases the mean upwards, and two
    different sequences of reordering operations can reach the same BDD, which double counts.
    Flooring pays for both. `--no-floor` shows the unfloored value for comparison.

Only the max-tput solutions are read. The gallium ones are a different question: that heuristic
models a controller-heavy design -- gallium-hyperloglog puts all three of its stateful objects on
the controller and runs at 89 pps, against 2.25 Gpps for the same NF's max-tput solution -- and
since only Tofino implementations are counted, such a plan records almost no alternatives: 8 of 83
nodes, so 10^3 where the other hyperloglog solutions are at 10^22. That number says where gallium
put the work, not how simple the NF is.
"""

from __future__ import annotations

import csv
import json
import math
import os
import statistics
import sys
from argparse import ArgumentParser
from pathlib import Path

from prettytable import PrettyTable

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / ".." / "..").resolve()
SYNTHESIZED_DIR = PROJECT_DIR / "synthesized"

NFS = ["cl", "fw", "nat", "kvs", "psd", "hyperloglog", "smartcookie"]


class Solution:
    def __init__(self, name: str, report: dict):
        self.name = name
        meta = report["search_meta"]
        self.per_node = {entry["node_id"]: entry["avg"] for entry in meta["avg_children_per_node"]}
        self.factors = list(self.per_node.values())
        self.steps = meta["steps"]
        self.explored = meta["ss_size"]
        self.reported_design_space = meta.get("log10_design_space")

    @property
    def nodes(self) -> int:
        return len(self.factors)

    def log10_size(self, floor: bool) -> float:
        """log10 of the product of the per-node number of implementations."""
        if floor and self.reported_design_space is not None:
            return self.reported_design_space
        total = 0.0
        for factor in self.factors:
            choices = math.floor(factor) if floor else factor
            if choices > 1:
                total += math.log10(choices)
        return total


def load(nf: str) -> list[Solution]:
    solutions = []
    for path in sorted(SYNTHESIZED_DIR.glob(f"{nf}-f40000-*.json")):
        with open(path) as f:
            report = json.load(f)
        if "search_meta" not in report:
            continue
        solutions.append(Solution(path.stem, report))
    return solutions


def size(log10_value: float) -> str:
    """Sizes are printed as the power of ten they are, since the numbers themselves do not fit.
    Rounded down, so the printed exponent is always one the estimate actually reaches."""
    return f"{math.floor(log10_value)}"


class Stats:
    def __init__(self, values: list[float]):
        self.min = min(values)
        self.max = max(values)
        self.mean = statistics.fmean(values)
        self.median = statistics.median(values)
        self.stdev = statistics.stdev(values) if len(values) > 1 else 0.0


def main() -> int:
    parser = ArgumentParser(description=__doc__.strip().splitlines()[0])
    parser.add_argument("--nfs", choices=NFS, nargs="+", default=NFS)
    parser.add_argument("--no-floor", action="store_true", help="do not floor the per-node counts (less conservative)")
    parser.add_argument("--per-workload", action="store_true", help="one row per workload instead of per NF")
    parser.add_argument("--csv", type=Path, help="also write the per-workload numbers to this file")
    args = parser.parse_args()

    floor = not args.no_floor
    per_nf: dict[str, list[Solution]] = {}
    for nf in args.nfs:
        solutions = load(nf)
        if not solutions:
            print(f"warning: no reports for {nf} in {SYNTHESIZED_DIR}", file=sys.stderr)
            continue
        per_nf[nf] = solutions

    if not per_nf:
        print("no reports found; synthesize the solutions first (tools/synapse_batcher.py)", file=sys.stderr)
        return 1

    unfloored = "" if floor else ", with the per-node counts NOT floored"

    if args.per_workload:
        table = PrettyTable()
        table.title = f"Implementations to choose between, per workload{unfloored}"
        table.field_names = ["NF", "workload", "BDD nodes", "implementations"]
        table.align = "r"
        table.align["NF"] = table.align["workload"] = "l"
        table.add_row(["", "", "", "(log10)"], divider=True)
        for nf, solutions in per_nf.items():
            for solution in solutions:
                table.add_row([nf, solution.name, solution.nodes, size(solution.log10_size(floor))])
        print(table)
    else:
        table = PrettyTable()
        table.title = f"Implementations to choose between, per NF{unfloored}"
        table.field_names = [
            "NF",
            "BDD nodes",
            "min",
            "median",
            "max",
            "mean",
            "stdev",
        ]
        table.align = "r"
        table.align["NF"] = "l"
        table.add_row(["", "", *["(log10)"] * 5], divider=True)

        def row(label: str, values: list[float], nodes: str) -> list:
            sizes = Stats(values)
            return [
                label,
                nodes,
                size(sizes.min),
                size(sizes.median),
                size(sizes.max),
                size(sizes.mean),
                f"{sizes.stdev:.0f}",
            ]

        every_size: list[float] = []
        rows = []
        for nf, solutions in per_nf.items():
            values = [s.log10_size(floor) for s in solutions]
            nodes = f"{statistics.median([s.nodes for s in solutions]):.0f}"
            rows.append(row(nf, values, nodes))
            every_size += values

        for fields in rows[:-1]:
            table.add_row(fields)
        table.add_row(rows[-1], divider=True)
        table.add_row(row("all", every_size, ""))
        print(table)

    if args.csv:
        with open(args.csv, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["nf", "workload", "bdd_nodes", "log10_design_space", "search_steps", "explored_plans"])
            for nf, solutions in per_nf.items():
                for solution in solutions:
                    writer.writerow([nf, solution.name, solution.nodes, size(solution.log10_size(floor)), solution.steps, solution.explored])
        print(f"\nwrote {args.csv}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
