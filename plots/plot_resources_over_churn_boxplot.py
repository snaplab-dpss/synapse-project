#!/usr/bin/env python3

from argparse import ArgumentParser
from itertools import product

import matplotlib.pyplot as plt
import numpy as np

from plot_resources_over_churn import (
    DEFAULT_CHURN_FPM,
    DEFAULT_TOTAL_FLOWS,
    DEFAULT_ZIPF_PARAMS,
    PLOTS_DIR,
    SYNTHESIZED_DIR,
    TARGET_NFS,
    build_synapse_nf_name,
)
from utils.tofino_resource_parser import parse_tofino_resources_file
from utils.util import whole_number_to_label
from utils.plot_config import *

OUTPUT_FILE = PLOTS_DIR / "resources_over_churn_boxplot.pdf"

BOX_COLOR = "#3D87FF"
LINE_COLOR = "#293132"


def plot(stages_per_churn: dict[int, list[float]]):
    churns = sorted(stages_per_churn)
    positions = np.arange(len(churns))

    fig, ax = plt.subplots(constrained_layout=True)

    ax.set_ylim(ymin=0, ymax=100)
    ax.set_ylabel("Stages (\\%)")
    ax.set_yticks(np.arange(0, 100 + 1, 100 / 5))

    line = dict(color=LINE_COLOR, linewidth=0.8)
    # Whiskers at min and max: matplotlib's default (1.5x the interquartile range, with outliers drawn as
    # points) is not how readers interpret a box plot.
    ax.boxplot(
        [stages_per_churn[churn] for churn in churns],
        positions=positions,
        widths=0.5,
        whis=(0, 100),
        patch_artist=True,
        boxprops=dict(facecolor=BOX_COLOR + "80", edgecolor=LINE_COLOR, linewidth=0.8),
        medianprops=dict(color=LINE_COLOR, linewidth=1.5),
        whiskerprops=line,
        capprops=line,
    )

    ax.set_xlabel("Churn (fpm)")
    ax.set_xticks(positions, [whole_number_to_label(churn) for churn in churns])
    ax.tick_params(axis="both", length=0)
    ax.grid(visible=False, axis="x")

    fig.set_size_inches(width, height * 0.65)

    print("-> ", OUTPUT_FILE)
    plt.savefig(str(OUTPUT_FILE), bbox_inches="tight", pad_inches=0)


if __name__ == "__main__":
    parser = ArgumentParser(description=f"Stage usage over churn, one box per churn pooling every NF's solutions. Synthesized dir: {SYNTHESIZED_DIR}.")

    parser.add_argument("--nfs", type=str, choices=TARGET_NFS, nargs="+", default=TARGET_NFS, help="Target NFs")
    parser.add_argument("--total-flows", type=int, nargs="+", default=DEFAULT_TOTAL_FLOWS, help="Total flows to generate")
    parser.add_argument("--zipf-params", type=float, nargs="+", default=DEFAULT_ZIPF_PARAMS, help="Zipf parameters")
    parser.add_argument("--churns", type=int, nargs="+", default=DEFAULT_CHURN_FPM, help="Churn rate (fpm)")

    args = parser.parse_args()

    stages_per_churn: dict[int, list[float]] = {churn: [] for churn in args.churns}

    for nf, total_flows, churn, zipf in product(args.nfs, args.total_flows, args.churns, args.zipf_params):
        resources_file = SYNTHESIZED_DIR / f"{build_synapse_nf_name(nf, total_flows, churn, zipf)}-resources.txt"
        assert resources_file.exists(), f"Synthesized resources file {resources_file} does not exist!"
        stages_per_churn[churn].append(parse_tofino_resources_file(resources_file).stages * 100)

    plot(stages_per_churn)
