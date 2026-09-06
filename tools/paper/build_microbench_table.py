#!/usr/bin/env python3

import os

from dataclasses import dataclass
from pathlib import Path
from argparse import ArgumentParser
from itertools import product
from statistics import mean, stdev, median, quantiles
from prettytable import PrettyTable

from synapse_compilation_report import parse_synapse_compilation_report

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / ".." / "..").resolve()

SYNTHESIZED_DIR = PROJECT_DIR / "synthesized"
TARGET_NFS = ["kvs", "fw", "nat", "psd", "cl", "hyperloglog"]

# Table labels; NFs missing from this map are shown as their upper-cased name.
NF_LABELS = {"hyperloglog": "HLL"}

DEFAULT_TOTAL_FLOWS = [40_000]
DEFAULT_CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
DEFAULT_ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2]


@dataclass
class Microbench:
    compilation_times: tuple[int, int] = (0, 0)
    backtracks: tuple[int, int] = (0, 0)
    speculated: tuple[int, int] = (0, 0)
    instantiated: tuple[int, int] = (0, 0)
    phase1_speculations: tuple[int, int] = (0, 0)
    phase2_speculations: tuple[int, int] = (0, 0)
    time_per_speculation_us: tuple[float, tuple[float, float]] = (0.0, (0.0, 0.0))
    time_per_instantiation_us: tuple[float, tuple[float, float]] = (0.0, (0.0, 0.0))


def build_synapse_nf_name(nf: str, total_flows: int, churn: int, zipf: float) -> str:
    dist = f"{'unif' if zipf == 0.0 else 'zipf'}{str(int(zipf) if int(zipf) == zipf else zipf).replace('.', '_') if zipf != 0.0 else ''}"
    heuristic = "max-tput"
    return f"{nf}-f{total_flows}-c{churn}-{dist}-h{heuristic}"


def multiline_latex_cell(lines: list[str]) -> str:
    content = r"\\ ".join(lines)
    return r"\begin{tabular}[c]{@{}l@{}}" + content + r"\end{tabular}"


def thousands_label(n: float) -> str:
    if n < 1000:
        return f"{n:.0f}"
    return f"{round(n / 1000):,}k"


def build_latex_table(microbench_per_nf: dict[str, Microbench]) -> str:
    prefix = r"\begin{tabular}{lccccccc}" + "\n"
    prefix += r"\toprule" + "\n"
    prefix += r"& " + multiline_latex_cell([r"\textbf{Total}", r"\textbf{Time}"])
    prefix += r"& " + multiline_latex_cell([r"\textbf{Spec.}", r"\textbf{S.S.}"])
    prefix += r"& " + multiline_latex_cell([r"\textbf{Comm.}", r"\textbf{S.S.}"])
    prefix += r"& " + multiline_latex_cell([r"\textbf{Local}", r"\textbf{Spec.}"])
    prefix += r"& " + multiline_latex_cell([r"\textbf{Recur.}", r"\textbf{Spec.}"])
    prefix += r"& " + multiline_latex_cell([r"\textbf{Time per}", r"\textbf{Spec.}"])
    prefix += r"& " + multiline_latex_cell([r"\textbf{Time per}", r"\textbf{Commit}"])
    prefix += r"\\" + "\n"
    prefix += r"\midrule" + "\n"

    rows = []

    for nf, mb in microbench_per_nf.items():
        time_per_spec_us = mb.time_per_speculation_us
        time_per_commit_ms = (mb.time_per_instantiation_us[0] / 1000, (mb.time_per_instantiation_us[1][0] / 1000, mb.time_per_instantiation_us[1][1] / 1000))

        columns = [
            NF_LABELS.get(nf, nf.upper()),
            f"\\evalue{{{mb.compilation_times[0]:.0f}s}}{{{mb.compilation_times[1]:.0f}}}",
            f"\\evalue{{{thousands_label(mb.speculated[0])}}}{{{thousands_label(mb.speculated[1])}}}",
            f"\\evalue{{{mb.instantiated[0]:,.0f}}}{{{mb.instantiated[1]:,.0f}}}",
            f"\\evalue{{{thousands_label(mb.phase1_speculations[0])}}}{{{thousands_label(mb.phase1_speculations[1])}}}",
            f"\\evalue{{{mb.phase2_speculations[0]:,.0f}}}{{{mb.phase2_speculations[1]:,.0f}}}",
            f"\\evalued{{{time_per_spec_us[0]:,.0f}\\textmu{{}}s}}{{{time_per_spec_us[1][0]:,.0f}}}{{{time_per_spec_us[1][1]:,.0f}}}",
            f"\\evalued{{{time_per_commit_ms[0]:,.1f}ms}}{{{time_per_commit_ms[1][0]:,.1f}}}{{{time_per_commit_ms[1][1]:,.1f}}}",
        ]

        row = " & ".join(columns) + r" \\"
        rows.append(row)

    content = "\n".join(rows)

    suffix = r"""
\bottomrule
\end{tabular}
"""

    return prefix + content + suffix


def get_p5_p95(data: list[float | int]) -> tuple[float, float]:
    q = quantiles(data, n=100)
    p5 = q[5]
    p95 = q[95]
    return p5, p95


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
    time_per_speculation_per_nf = {nf: [] for nf in args.nfs}
    time_per_instantiation_per_nf = {nf: [] for nf in args.nfs}

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
            time_per_speculation_per_nf[nf].append(report.global_stats.time_per_speculation_us)
            time_per_instantiation_per_nf[nf].append(report.global_stats.time_per_instantiation_us)

    data = {nf: Microbench() for nf in args.nfs}

    for nf in args.nfs:
        data[nf].compilation_times = (mean(compilation_times_per_nf[nf]), stdev(compilation_times_per_nf[nf]))
        data[nf].backtracks = (mean(backtracks_per_nf[nf]), stdev(backtracks_per_nf[nf]))
        data[nf].speculated = (mean(speculated_per_nf[nf]), stdev(speculated_per_nf[nf]))
        data[nf].instantiated = (mean(instantiated_per_nf[nf]), stdev(instantiated_per_nf[nf]))
        data[nf].phase1_speculations = (mean(phase1_speculations_per_nf[nf]), stdev(phase1_speculations_per_nf[nf]))
        data[nf].phase2_speculations = (mean(phase2_speculations_per_nf[nf]), stdev(phase2_speculations_per_nf[nf]))
        data[nf].time_per_speculation_us = (median(time_per_speculation_per_nf[nf]), get_p5_p95(time_per_speculation_per_nf[nf]))
        data[nf].time_per_instantiation_us = (median(time_per_instantiation_per_nf[nf]), get_p5_p95(time_per_instantiation_per_nf[nf]))

    table = PrettyTable()
    table.field_names = [
        "NF",
        "Time (s)",
        # "Backtracks",
        "Speculated EPs",
        "Instantiated EPs",
        "Spec Phase 1",
        "Spec Phase 2",
        "Time per Speculation (us)",
        "Time per Instantiation (us)",
    ]
    table.align = "l"

    for nf in args.nfs:
        avg_phase2_speculations_rel = data[nf].phase2_speculations[0] / data[nf].phase1_speculations[0] if data[nf].phase1_speculations[0] > 0 else 0
        stdev_phase2_speculations_rel = data[nf].phase2_speculations[1] / data[nf].phase1_speculations[0] if data[nf].phase1_speculations[0] > 0 else 0

        table.add_row(
            [
                NF_LABELS.get(nf, nf.upper()),
                f"{data[nf].compilation_times[0]:,.0f} ± {data[nf].compilation_times[1]:,.0f}",
                # f"{avg_backtracks:,.0f} ± {stdev_backtracks:,.0f}",
                f"{data[nf].speculated[0]:,.0f} ± {data[nf].speculated[1]:,.0f}",
                f"{data[nf].instantiated[0]:,.0f} ± {data[nf].instantiated[1]:,.0f}",
                f"{data[nf].phase1_speculations[0]:,.0f} ± {data[nf].phase1_speculations[1]:,.0f}",
                f"{data[nf].phase2_speculations[0]:,.0f} ± {data[nf].phase2_speculations[1]:,.0f} ({100*avg_phase2_speculations_rel:.2f} ± {100*stdev_phase2_speculations_rel:.2f} %)",
                f"{data[nf].time_per_speculation_us[0]:,.0f} (p5={data[nf].time_per_speculation_us[1][0]:,.0f} p95={data[nf].time_per_speculation_us[1][1]:,.0f})",
                f"{data[nf].time_per_instantiation_us[0]:,.0f} (p5={data[nf].time_per_instantiation_us[1][0]:,.0f} p95={data[nf].time_per_instantiation_us[1][1]:,.0f})",
            ]
        )
    print(table)

    latex_table = build_latex_table(data)
    print(latex_table)
