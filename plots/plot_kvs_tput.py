#!/usr/bin/env python3

import os

from pathlib import Path

from utils.parser import parse_heatmap_data_file
from utils.plot_config import *
from utils.heatmap import *

import matplotlib.pyplot as plt

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

OUTPUT_FILE = PLOTS_DIR / "kvs_throughput.pdf"

TPUT_MPPS_MIN = 0
TPUT_MPPS_MAX = 2500

# SYSTEM_NAME = "Synapse"
SYSTEM_NAME = "Tessera"

nf = "KVS"

solutions = {
    "Gallium": DATA_DIR / "tput_gallium_kvs.csv",
    "NetCache": DATA_DIR / "tput_netcache.csv",
    "Switcharoo": DATA_DIR / "tput_switcharoo.csv",
    SYSTEM_NAME: DATA_DIR / "tput_synapse_kvs.csv",
}

CHOSEN_WORKLOADS = [Key(s=1.2, churn_fpm=c) for c in [0, 1_000, 10_000, 100_000, 1_000_000]]


def parse_data_files():
    data = {}

    for solution, data_file in solutions.items():
        data[solution] = {
            "y": [],
            "yerr": [],
        }

        solution_data = parse_heatmap_data_file(data_file)
        filtered_data = solution_data.filter(CHOSEN_WORKLOADS)

        y = [x.dut_egress_pps for x in filtered_data.get_avg_values().values()]
        yerr = [x.dut_egress_pps for x in filtered_data.get_stdev_values().values()]

        data[solution]["y"] = y
        data[solution]["yerr"] = yerr

    return data


def plot(data: dict):
    ind = np.arange(len(CHOSEN_WORKLOADS))
    bar_width = 0.15

    fig, ax = plt.subplots(constrained_layout=True)

    ax.set_ylim(ymin=1, ymax=TPUT_MPPS_MAX)
    ax.set_ylabel("Tput (Mpps)")
    ax.set_yticks(np.arange(0, TPUT_MPPS_MAX + 1, TPUT_MPPS_MAX / 5))

    colors = [
        "#2400D8",
        "#3D87FF",
        "#FF7F00",
        "#FF3D3D",
    ]

    pos = ind
    for (sol, throughput_per_workload), hatch, color in zip(data.items(), itertools.cycle(hatch_list), itertools.cycle(colors)):
        y_Mpps = [y / 1e6 for y in throughput_per_workload["y"]]
        yerr_Mpps = [yerr / 1e6 for yerr in throughput_per_workload["yerr"]]
        ax.bar(pos, y_Mpps, bar_width, yerr=yerr_Mpps, label=sol, alpha=0.99, hatch=hatch, error_kw=dict(lw=1, capsize=1, capthick=0.3), color=color)
        pos = pos + bar_width

    labels = [whole_number_to_label(key.churn_fpm) for key in CHOSEN_WORKLOADS]

    ax.set_xlabel("Churn (fpm)")
    ax.set_xticks(ind + bar_width * (3 / 2), labels)
    ax.tick_params(axis="both", length=0)

    ax.legend(bbox_to_anchor=(0.4, 1.35), loc="upper center", ncols=4, columnspacing=0.6, handletextpad=0.2, fontsize="small")
    fig.set_size_inches(width, height * 0.8)

    print("-> ", OUTPUT_FILE)
    plt.savefig(str(OUTPUT_FILE), bbox_inches="tight", pad_inches=0)


def main():
    data = parse_data_files()
    plot(data)


if __name__ == "__main__":
    main()
