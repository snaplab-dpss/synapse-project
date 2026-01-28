#!/usr/bin/env python3

import os

from pathlib import Path
from argparse import ArgumentParser
from itertools import product
from statistics import mean, stdev
from prettytable import PrettyTable

from synapse_compilation_report import parse_synapse_compilation_report

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / ".." / "..").resolve()

SYNTHESIZED_DIR = PROJECT_DIR / "synthesized"
TARGET_NFS = ["kvs", "fw", "nat", "psd", "cl"]

DEFAULT_TOTAL_FLOWS = [40_000]
DEFAULT_CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
DEFAULT_ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2]


def build_synapse_nf_name(nf: str, total_flows: int, churn: int, zipf: float) -> str:
    dist = f"{'unif' if zipf == 0.0 else 'zipf'}{str(int(zipf) if int(zipf) == zipf else zipf).replace('.', '_') if zipf != 0.0 else ''}"
    heuristic = "max-tput"
    return f"{nf}-f{total_flows}-c{churn}-{dist}-h{heuristic}"


if __name__ == "__main__":
    description = "Synapse batcher script. This will run synapse against a batch of NFs and profiling reports."
    description += f" Synthesized dir: {SYNTHESIZED_DIR}."

    parser = ArgumentParser(description=description)

    parser.add_argument("--nfs", type=str, choices=TARGET_NFS, nargs="+", default=TARGET_NFS, help="Target NFs")
    parser.add_argument("--total-flows", type=int, nargs="+", default=DEFAULT_TOTAL_FLOWS, help="Total flows to generate")
    parser.add_argument("--zipf-params", type=float, nargs="+", default=DEFAULT_ZIPF_PARAMS, help="Zipf parameters")
    parser.add_argument("--churns", type=int, nargs="+", default=DEFAULT_CHURN_FPM, help="Churn rate (fpm)")

    args = parser.parse_args()

    Path.mkdir(SYNTHESIZED_DIR, exist_ok=True)

    compilation_times_per_nf = {nf: [] for nf in args.nfs}
    backtracks_per_nf = {nf: [] for nf in args.nfs}
    speculated_per_nf = {nf: [] for nf in args.nfs}
    instantiated_per_nf = {nf: [] for nf in args.nfs}
    phase1_speculations_per_nf = {nf: [] for nf in args.nfs}
    phase2_speculations_per_nf = {nf: [] for nf in args.nfs}

    for nf in args.nfs:
        for total_flows, churn, zipf in product(args.total_flows, args.churns, args.zipf_params):
            nf_name = build_synapse_nf_name(nf, total_flows, churn, zipf)

            p4_file = SYNTHESIZED_DIR / f"{nf_name}.p4"
            synapse_compilation_report_file = SYNTHESIZED_DIR / f"{nf_name}.json"

            assert synapse_compilation_report_file.exists(), f"Synthesized synapse compilation report file {synapse_compilation_report_file} does not exist!"

            report = parse_synapse_compilation_report(synapse_compilation_report_file)
            compilation_times_per_nf[nf].append(report.search_meta.elapsed_time_seconds)
            backtracks_per_nf[nf].append(report.search_meta.backtracks)
            speculated_per_nf[nf].append(report.global_stats.num_speculated_modules)
            instantiated_per_nf[nf].append(report.global_stats.num_execution_plans_generated)
            phase1_speculations_per_nf[nf].append(report.global_stats.num_phase1_speculations)
            phase2_speculations_per_nf[nf].append(report.global_stats.num_phase2_speculations)

    avg_compilation_times_per_nf = {}
    avg_backtracks_per_nf = {}
    avg_speculated_per_nf = {}
    avg_instantiated_per_nf = {}
    avg_phase1_speculations_per_nf = {}
    avg_phase2_speculations_per_nf = {}

    for nf in args.nfs:
        avg_compilation_time = mean(compilation_times_per_nf[nf])
        stdev_compilation_time = stdev(compilation_times_per_nf[nf])
        avg_compilation_times_per_nf[nf] = (avg_compilation_time, stdev_compilation_time)

        avg_backtracks = int(mean(backtracks_per_nf[nf]))
        stdev_backtracks = int(stdev(backtracks_per_nf[nf]))
        avg_backtracks_per_nf[nf] = (avg_backtracks, stdev_backtracks)

        avg_speculated = int(mean(speculated_per_nf[nf]))
        stdev_speculated = int(stdev(speculated_per_nf[nf]))
        avg_speculated_per_nf[nf] = (avg_speculated, stdev_speculated)

        avg_instantiated = int(mean(instantiated_per_nf[nf]))
        stdev_instantiated = int(stdev(instantiated_per_nf[nf]))
        avg_instantiated_per_nf[nf] = (avg_instantiated, stdev_instantiated)

        avg_phase1_speculations = int(mean(phase1_speculations_per_nf[nf]))
        stdev_phase1_speculations = int(stdev(phase1_speculations_per_nf[nf]))
        avg_phase1_speculations_per_nf[nf] = (avg_phase1_speculations, stdev_phase1_speculations)

        avg_phase2_speculations = int(mean(phase2_speculations_per_nf[nf]))
        stdev_phase2_speculations = int(stdev(phase2_speculations_per_nf[nf]))
        avg_phase2_speculations_per_nf[nf] = (avg_phase2_speculations, stdev_phase2_speculations)

    table = PrettyTable()
    table.field_names = [
        "NF",
        "Time (s)",
        "Backtracks",
        "Speculated EPs",
        "Instantiated EPs",
        "Spec Phase 1",
        "Spec Phase 2",
    ]
    table.align = "l"

    for nf in args.nfs:
        avg_compilation_time, stdev_compilation_time = avg_compilation_times_per_nf[nf]
        avg_backtracks, stdev_backtracks = avg_backtracks_per_nf[nf]
        avg_speculated, stdev_speculated = avg_speculated_per_nf[nf]
        avg_instantiated, stdev_instantiated = avg_instantiated_per_nf[nf]
        avg_phase1_speculations, stdev_phase1_speculations = avg_phase1_speculations_per_nf[nf]
        avg_phase2_speculations, stdev_phase2_speculations = avg_phase2_speculations_per_nf[nf]

        avg_phase2_speculations_rel = avg_phase2_speculations / avg_phase1_speculations if avg_phase1_speculations > 0 else 0
        stdev_phase2_speculations_rel = stdev_phase2_speculations / avg_phase1_speculations if avg_phase1_speculations > 0 else 0

        table.add_row(
            [
                nf.upper(),
                f"{avg_compilation_time:.2f} ± {stdev_compilation_time:.2f}",
                f"{avg_backtracks:,} ± {stdev_backtracks:,}",
                f"{avg_speculated:,} ± {stdev_speculated:,}",
                f"{avg_instantiated:,} ± {stdev_instantiated:,}",
                f"{avg_phase1_speculations:,} ± {stdev_phase1_speculations:,}",
                f"{avg_phase2_speculations:,} ± {stdev_phase2_speculations:,} ({avg_phase2_speculations_rel*100:.2f} ± {stdev_phase2_speculations_rel*100:.2f} %)",
            ]
        )
    print(table)
