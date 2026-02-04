#!/usr/bin/env python3

import os

from pathlib import Path
from argparse import ArgumentParser
from itertools import product
from statistics import mean, stdev
from prettytable import PrettyTable

from tofino_resource_parser import calculate_lines_of_p4_code, parse_tofino_resources_file, Resources
from tput_eval_data import EvalDataKey

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / ".." / "..").resolve()

SYNTHESIZED_DIR = PROJECT_DIR / "synthesized"
NETCACHE_DIR = PROJECT_DIR / "tofino" / "netcache" / "p4"
SWITCHAROO_DIR = PROJECT_DIR / "tofino" / "switcharoo" / "p4"

TARGET_NFS = ["kvs", "fw", "nat", "psd", "cl"]

DEFAULT_TOTAL_FLOWS = [40_000]
DEFAULT_CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
DEFAULT_ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2]


def whole_number_to_label(n: int) -> str:
    if n < 1e3:
        return f"{n}"
    elif n < 1e6:
        return f"{int(round(n / 1e3, 2))}K"
    elif n < 1e9:
        return f"{int(round(n / 1e6, 2))}M"
    elif n < 1e12:
        return f"{int(round(n / 1e9, 2))}G"
    else:
        return f"{int(round(n / 1e12, 2))}T"


def build_synapse_nf_name(nf: str, total_flows: int, churn: int, zipf: float) -> str:
    dist = f"{'unif' if zipf == 0.0 else 'zipf'}{str(int(zipf) if int(zipf) == zipf else zipf).replace('.', '_') if zipf != 0.0 else ''}"
    heuristic = "max-tput"
    return f"{nf}-f{total_flows}-c{churn}-{dist}-h{heuristic}"


def build_resources_latex_table(avg_resources_per_nf: dict[str, tuple[Resources, Resources]], avg_loc_per_nf: dict[str, tuple[float, float]]) -> str:
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


def build_stages_over_churn_latex_table(resources_per_nf_per_key: dict[str, dict[EvalDataKey, Resources]]) -> str:
    churns = sorted(list(set([key.churn for nf_dict in resources_per_nf_per_key.values() for key in nf_dict.keys()])))
    prefix = (
        r"""
\begin{tabular}{l"""
        + "c" * len(churns)
        + r"""}
\toprule
"""
        + "& "
        + " & ".join([f"{whole_number_to_label(c)}fpm" for c in churns])
        + r""" \\
\midrule
"""
    )

    rows = []

    for nf in resources_per_nf_per_key.keys():
        row_elems = [nf.upper()]
        for c in churns:
            vals = []
            for key in resources_per_nf_per_key[nf].keys():
                if key.churn == c:
                    res = resources_per_nf_per_key[nf][key]
                    vals.append(res.stages)
            avg_val = mean(vals)
            stdev_val = stdev(vals) if len(vals) > 1 else 0
            row_elems.append(f"\\evalue{{{100*avg_val:.1f}}}{{{100*stdev_val:.1f}}}")
        row = " & ".join(row_elems) + r" \\"
        rows.append(row)

    content = "\n".join(rows)

    suffix = r"""
\bottomrule
\end{tabular}
"""

    return prefix + content + suffix


def build_kvs_resources_latex_table(
    netcache: Resources,
    switcharoo: Resources,
    gallium: Resources,
    synapse: tuple[Resources, Resources],
) -> str:
    prefix = r"""
\begin{tabular}{lcccc}
\toprule
& \textbf{Stages} & \textbf{SRAM} & \textbf{VLIW} & \begin{tabular}[c]{@{}l@{}}\textbf{Match}\\ \textbf{xbar}\end{tabular} \\
\cmidrule(r){2-5}
& \multicolumn{4}{c@{}}{\footnotesize \textit{(\%)}} \\
\midrule
"""

    rows = []

    rows.append(
        " & ".join(
            [
                "NetCache",
                f"{100*netcache.stages:.1f}",
                f"{100*netcache.sram:.1f}",
                f"{100*netcache.vliw:.1f}",
                f"{100*netcache.match_xbar:.1f}",
            ]
        )
        + r" \\"
    )

    rows.append(
        " & ".join(
            [
                "Switcharoo",
                f"{100*switcharoo.stages:.1f}",
                f"{100*switcharoo.sram:.1f}",
                f"{100*switcharoo.vliw:.1f}",
                f"{100*switcharoo.match_xbar:.1f}",
            ]
        )
        + r" \\"
    )

    rows.append(
        " & ".join(
            [
                "Gallium",
                f"{100*gallium.stages:.1f}",
                f"{100*gallium.sram:.1f}",
                f"{100*gallium.vliw:.1f}",
                f"{100*gallium.match_xbar:.1f}",
            ]
        )
        + r" \\"
    )

    rows.append(
        " & ".join(
            [
                "Tessera",
                f"\\evalue{{{100*synapse[0].stages:.1f}}}{{{100*synapse[1].stages:.1f}}}",
                f"\\evalue{{{100*synapse[0].sram:.1f}}}{{{100*synapse[1].sram:.1f}}}",
                f"\\evalue{{{100*synapse[0].vliw:.1f}}}{{{100*synapse[1].vliw:.1f}}}",
                f"\\evalue{{{100*synapse[0].match_xbar:.1f}}}{{{100*synapse[1].match_xbar:.1f}}}",
            ]
        )
        + r" \\"
    )

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
    Path.mkdir(NETCACHE_DIR, exist_ok=True)
    Path.mkdir(SWITCHAROO_DIR, exist_ok=True)

    resources_per_nf = {nf: [] for nf in args.nfs}
    loc_per_nf = {nf: [] for nf in args.nfs}
    resources_per_nf_per_key: dict[str, dict[EvalDataKey, Resources]] = {nf: {} for nf in args.nfs}

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

            key = EvalDataKey(skew=zipf, churn=churn)
            resources_per_nf_per_key[nf][key] = resources

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

        total_avg = avg_resources.get_total_avg()
        total_stddev = avg_resources.get_total_stddev()

    table = PrettyTable()
    table.field_names = ["NF", "LoC", "Stages (%)", "SRAM (%)", "VLIW (%)", "Match Xbar (%)", "Average (%)"]
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
                f"{100*avg_resources.get_total_avg():4.1f} ± {100*avg_resources.get_total_stddev():4.1f}",
            ]
        )
    print(table)

    table2 = PrettyTable()
    table2.field_names = ["NF"] + [f"{c:,}fpm" for c in args.churns]
    for nf in args.nfs:
        row = []
        for c in args.churns:
            vals = []
            for s in args.zipf_params:
                key = EvalDataKey(skew=s, churn=c)
                res = resources_per_nf_per_key[nf][key]
                vals.append(res.stages)
            avg_val = mean(vals)
            stdev_val = stdev(vals) if len(vals) > 1 else 0
            row.append(f"{100*avg_val:4.1f} ± {100*stdev_val:4.1f}")
        table2.add_row([nf] + row)
    print(table2)

    table3 = PrettyTable()
    table3.field_names = ["NF"] + [f"s={c:,}" for c in args.zipf_params]
    for nf in args.nfs:
        row = []
        for s in args.zipf_params:
            vals = []
            for c in args.churns:
                key = EvalDataKey(skew=s, churn=c)
                res = resources_per_nf_per_key[nf][key]
                vals.append(res.stages)
            avg_val = mean(vals)
            stdev_val = stdev(vals) if len(vals) > 1 else 0
            row.append(f"{100*avg_val:4.1f} ± {100*stdev_val:4.1f}")
        table3.add_row([nf] + row)
    print(table3)

    latex_table = build_resources_latex_table(avg_resources_per_nf, avg_loc_per_nf)
    print(latex_table)

    latex_table2 = build_stages_over_churn_latex_table(resources_per_nf_per_key)
    print(latex_table2)

    netcache_resources_file = NETCACHE_DIR / "netcache-resources.txt"
    switcharoo_resources_file = SWITCHAROO_DIR / "switcharoo-resources.txt"
    gallium_resources_file = SYNTHESIZED_DIR / "gallium-kvs-resources.txt"

    assert netcache_resources_file.exists(), f"Synthesized resources file {netcache_resources_file} does not exist!"
    assert switcharoo_resources_file.exists(), f"Synthesized resources file {switcharoo_resources_file} does not exist!"
    assert gallium_resources_file.exists(), f"Synthesized resources file {gallium_resources_file} does not exist!"

    netcache_resources = parse_tofino_resources_file(netcache_resources_file)
    switcharoo_resources = parse_tofino_resources_file(switcharoo_resources_file)
    gallium_resources = parse_tofino_resources_file(gallium_resources_file)

    latex_table3 = build_kvs_resources_latex_table(
        netcache=netcache_resources,
        switcharoo=switcharoo_resources,
        gallium=gallium_resources,
        synapse=avg_resources_per_nf["kvs"],
    )
    print(latex_table3)
