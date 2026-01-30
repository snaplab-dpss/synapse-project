#!/usr/bin/env python3

import os

from pathlib import Path
from argparse import ArgumentParser
from itertools import product
from statistics import mean, stdev
from prettytable import PrettyTable

from tofino_resource_parser import calculate_lines_of_p4_code, parse_tofino_resources_file, Resources

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


def build_latex_table(avg_resources_per_nf: dict[str, tuple[Resources, Resources]], avg_loc_per_nf: dict[str, tuple[float, float]]) -> str:
    # \begin{tabular}{lccccc}
    # \toprule
    # & \textbf{P4} & \textbf{Stages} & \textbf{SRAM} & \textbf{VLIW} & \begin{tabular}[c]{@{}l@{}}\textbf{Match}\\ \textbf{xbar}\end{tabular} \\
    # \cmidrule(r){2-2} \cmidrule(r){3-6}
    # & {\scriptsize \textit{(LoC)}} & \multicolumn{4}{c@{}}{\footnotesize \textit{(\%)}} \\
    # \midrule
    # \textbf{KVS} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} \\
    # \textbf{FW}  & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} \\
    # \textbf{NAT} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} \\
    # \textbf{LB}  & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} \\
    # \textbf{CL}  & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} \\
    # \textbf{PSD} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} & \evalue{1.5}{0.2} \\
    # \bottomrule
    # \end{tabular}

    prefix = r"""
\begin{tabular}{lccccc}
\toprule
& \textbf{P4} & \textbf{Stages} & \textbf{SRAM} & \textbf{VLIW} & \begin{tabular}[c]{@{}l@{}}\textbf{Match}\\ \textbf{xbar}\end{tabular} \\
\cmidrule(r){2-2} \cmidrule(r){3-6}
& {\scriptsize \textit{(LoC)}} & \multicolumn{4}{c@{}}{\footnotesize \textit{(\%)}} \\
\midrule
"""

    rows = []

    for nf in avg_resources_per_nf.keys():
        assert nf in avg_loc_per_nf, f"NF {nf} missing in loc data!"
        avg_resources, stdev_resources = avg_resources_per_nf[nf]
        avg_loc, stdev_loc = avg_loc_per_nf[nf]

        columns = [
            nf.upper(),
            f"\\evalue{{{avg_loc}}}{{{stdev_loc}}}",
            f"\\evalue{{{100*avg_resources.stages:.1f}}}{{{100*stdev_resources.stages:.1f}}}",
            f"\\evalue{{{100*avg_resources.sram:.1f}}}{{{100*stdev_resources.sram:.1f}}}",
            f"\\evalue{{{100*avg_resources.vliw:.1f}}}{{{100*stdev_resources.vliw:.1f}}}",
            f"\\evalue{{{100*avg_resources.match_xbar:.1f}}}{{{100*stdev_resources.match_xbar:.1f}}}",
        ]

        row = " & ".join(columns) + r" \\"
        rows.append(row)

    content = "\n".join(rows)

    suffix = r"""
\bottomrule
\end{tabular}
"""

    return prefix + content + suffix


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

    resources_per_nf = {nf: [] for nf in args.nfs}
    loc_per_nf = {nf: [] for nf in args.nfs}

    for nf in args.nfs:
        for total_flows, churn, zipf in product(args.total_flows, args.churns, args.zipf_params):
            nf_name = build_synapse_nf_name(nf, total_flows, churn, zipf)

            p4_file = SYNTHESIZED_DIR / f"{nf_name}.p4"
            resources_file = SYNTHESIZED_DIR / f"{nf_name}-resources.txt"

            assert p4_file.exists(), f"Synthesized P4 file {p4_file} does not exist!"
            assert resources_file.exists(), f"Synthesized resources file {resources_file} does not exist!"

            resources = parse_tofino_resources_file(resources_file)
            loc = calculate_lines_of_p4_code(p4_file)

            resources_per_nf[nf].append(resources)
            loc_per_nf[nf].append(loc)

    avg_resources_per_nf = {}
    avg_loc_per_nf = {}

    for nf in args.nfs:
        avg_resources = Resources(
            stages=mean([r.stages for r in resources_per_nf[nf]]),
            sram=mean([r.sram for r in resources_per_nf[nf]]),
            vliw=mean([r.vliw for r in resources_per_nf[nf]]),
            match_xbar=mean([r.match_xbar for r in resources_per_nf[nf]]),
        )
        stdev_resources = Resources(
            stages=stdev([r.stages for r in resources_per_nf[nf]]),
            sram=stdev([r.sram for r in resources_per_nf[nf]]),
            vliw=stdev([r.vliw for r in resources_per_nf[nf]]),
            match_xbar=stdev([r.match_xbar for r in resources_per_nf[nf]]),
        )

        avg_loc = int(mean(loc_per_nf[nf]))
        stdev_loc = int(stdev(loc_per_nf[nf]))

        avg_resources_per_nf[nf] = (avg_resources, stdev_resources)
        avg_loc_per_nf[nf] = (avg_loc, stdev_loc)

    table = PrettyTable()
    table.field_names = ["NF", "LoC", "Stages (%)", "SRAM (%)", "VLIW (%)", "Match Xbar (%)"]
    for nf in args.nfs:
        avg_resources, stdev_resources = avg_resources_per_nf[nf]
        avg_loc, stdev_loc = avg_loc_per_nf[nf]
        table.add_row(
            [
                nf,
                f"{avg_loc} ± {stdev_loc}",
                f"{100*avg_resources.stages:4.1f} ± {100*stdev_resources.stages:4.1f}",
                f"{100*avg_resources.sram:4.1f} ± {100*stdev_resources.sram:4.1f}",
                f"{100*avg_resources.vliw:4.1f} ± {100*stdev_resources.vliw:4.1f}",
                f"{100*avg_resources.match_xbar:4.1f} ± {100*stdev_resources.match_xbar:4.1f}",
            ]
        )
    print(table)

    latex_table = build_latex_table(avg_resources_per_nf, avg_loc_per_nf)
    print(latex_table)
