#!/usr/bin/env python3

import os

from pathlib import Path
from argparse import ArgumentParser
from itertools import product
from statistics import mean, stdev
from prettytable import PrettyTable

from utils.tofino_resource_parser import calculate_lines_of_p4_code, parse_tofino_resources_file, Resources
from utils.parser import parse_heatmap_data_file
from utils.plot_config import *
from utils.heatmap import *

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / "..").resolve()
PLOTS_DIR = CURRENT_DIR / "plots"

SYNTHESIZED_DIR = PROJECT_DIR / "synthesized"

TARGET_NFS = ["fw", "nat", "psd", "cl", "kvs"]

DEFAULT_TOTAL_FLOWS = [40_000]
DEFAULT_CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
DEFAULT_ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2]

OUTPUT_FILE = PLOTS_DIR / "resources_over_churn.pdf"


def build_synapse_nf_name(nf: str, total_flows: int, churn: int, zipf: float) -> str:
    dist = f"{'unif' if zipf == 0.0 else 'zipf'}{str(int(zipf) if int(zipf) == zipf else zipf).replace('.', '_') if zipf != 0.0 else ''}"
    heuristic = "max-tput"
    return f"{nf}-f{total_flows}-c{churn}-{dist}-h{heuristic}"


def plot(data: dict[str, dict[int, tuple[Resources, Resources]]]):
    churns = sorted(list(set(churn for nf_data in data.values() for churn in nf_data)))

    ind = np.arange(len(data))
    bar_width = 0.15

    fig, ax = plt.subplots(constrained_layout=True)

    ax.set_ylim(ymin=0, ymax=100)
    ax.set_ylabel("Stages (\\%)")
    ax.set_yticks(np.arange(0, 100 + 1, 100 / 5))

    colors = [
        "#2400D8",
        "#3D87FF",
        "#FF7F00",
        "#FF3D3D",
        "#293132",
    ]

    pos = ind
    for (nf, nf_data), hatch, color in zip(data.items(), itertools.cycle(hatch_list), itertools.cycle(colors)):
        ys = []
        yerrs = []
        for churn in churns:
            ys.append(nf_data[churn][0].stages * 100)
            yerrs.append(nf_data[churn][1].stages * 100)
        ax.bar(pos, ys, bar_width, yerr=yerrs, label=nf.upper(), alpha=0.99, hatch=hatch, error_kw=dict(lw=1, capsize=1, capthick=0.3), color=color)
        pos = pos + bar_width

    labels = [whole_number_to_label(churn) for churn in churns]

    ax.set_xlabel("Churn (fpm)")
    ax.set_xticks(ind + bar_width * 2, labels)
    ax.tick_params(axis="both", length=0)

    ax.legend(bbox_to_anchor=(0.4, 1.35), loc="upper center", ncols=5, columnspacing=0.6, handletextpad=0.2, fontsize="small")
    fig.set_size_inches(width, height * 0.8)

    print("-> ", OUTPUT_FILE)
    plt.savefig(str(OUTPUT_FILE), bbox_inches="tight", pad_inches=0)


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

    resources_per_nf_per_churn = {nf: {} for nf in args.nfs}

    for nf in args.nfs:
        for total_flows, churn, zipf in product(args.total_flows, args.churns, args.zipf_params):
            nf_name = build_synapse_nf_name(nf, total_flows, churn, zipf)

            p4_file = SYNTHESIZED_DIR / f"{nf_name}.p4"
            resources_file = SYNTHESIZED_DIR / f"{nf_name}-resources.txt"

            assert p4_file.exists(), f"Synthesized P4 file {p4_file} does not exist!"
            assert resources_file.exists(), f"Synthesized resources file {resources_file} does not exist!"

            resources = parse_tofino_resources_file(resources_file)

            if churn not in resources_per_nf_per_churn[nf]:
                resources_per_nf_per_churn[nf][churn] = []
            resources_per_nf_per_churn[nf][churn].append(resources)

    avg_resources_per_nf_per_churn = {nf: {} for nf in args.nfs}

    for nf in args.nfs:
        for churn in resources_per_nf_per_churn[nf]:
            avg_resources = Resources(
                stages=mean([r.stages for r in resources_per_nf_per_churn[nf][churn]]),
                sram=mean([r.sram for r in resources_per_nf_per_churn[nf][churn]]),
                vliw=mean([r.vliw for r in resources_per_nf_per_churn[nf][churn]]),
                match_xbar=mean([r.match_xbar for r in resources_per_nf_per_churn[nf][churn]]),
            )
            stdev_resources = Resources(
                stages=stdev([r.stages for r in resources_per_nf_per_churn[nf][churn]]),
                sram=stdev([r.sram for r in resources_per_nf_per_churn[nf][churn]]),
                vliw=stdev([r.vliw for r in resources_per_nf_per_churn[nf][churn]]),
                match_xbar=stdev([r.match_xbar for r in resources_per_nf_per_churn[nf][churn]]),
            )

            avg_resources_per_nf_per_churn[nf][churn] = (avg_resources, stdev_resources)

    plot(avg_resources_per_nf_per_churn)
