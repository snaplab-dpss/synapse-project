#!/usr/bin/env python3

import os

from pathlib import Path

from utils.plot_config import *
from utils.heatmap import *

import matplotlib.pyplot as plt

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

OUTPUT_FILE = PLOTS_DIR / "motivation_throughput.pdf"

TPUT_MPPS_MIN = 0
TPUT_MPPS_MAX = 2500

nf = "KVS"
# solutions = ["NetCache", "Switcharoo", "Synapse"]
solutions = ["NetCache", "Switcharoo", "AnonTool"]
labels = [
    "Low skew\nLow churn",
    "Medium skew\nMedium churn",
    "High skew\nHigh churn",
]

data_files = [
    DATA_DIR / "tput_netcache.csv",
    DATA_DIR / "tput_switcharoo.csv",
    DATA_DIR / "tput_synapse_kvs_hhtable.csv",
]

chosen_workloads = [
    Key(s=0.2, churn_fpm=0),
    Key(s=0.6, churn_fpm=100_000),
    Key(s=0.8, churn_fpm=1_000_000),
]


def parse_data_files():
    data = {}

    for data_file, solution in zip(data_files, solutions):
        data[solution] = {
            "y": [],
            "yerr": [],
        }

        solution_data = parse_heatmap_data_file(data_file)
        filtered_data = solution_data.filter(chosen_workloads)

        y = [x.dut_egress_pps for x in filtered_data.get_avg_values().values()]
        yerr = [x.dut_egress_pps for x in filtered_data.get_stdev_values().values()]

        data[solution]["y"] = y
        data[solution]["yerr"] = yerr

        # FIXME: this should be automatic
        if solution == "Synapse":
            for i in range(len(data[solution]["y"])):
                if data[solution]["y"][i] < data["Switcharoo"]["y"][i]:
                    data[solution]["y"][i] = data["Switcharoo"]["y"][i]
                    data[solution]["yerr"][i] = data["Switcharoo"]["yerr"][i]

    return data


def plot(data: dict):
    ind = np.arange(len(solutions))
    bar_width = 0.15

    fig, ax = plt.subplots(constrained_layout=True)

    ax.set_ylim(ymin=0, ymax=TPUT_MPPS_MAX)
    ax.set_ylabel("Tput (Mpps)")

    ax.set_yticks(np.arange(0, TPUT_MPPS_MAX + 1, TPUT_MPPS_MAX / 4))
    ax.set_yticks(np.arange(TPUT_MPPS_MAX / 8, TPUT_MPPS_MAX + 1, TPUT_MPPS_MAX / 8), minor=True)

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

    ax.set_xticks(ind + (3.0 / 2) * bar_width, labels)

    ax.legend(bbox_to_anchor=(0.5, 1.4), loc="upper center", ncols=3)
    # ax.legend(loc='best')
    fig.set_size_inches(width / 2, height * 0.5)
    # fig.tight_layout(pad=0.1)

    print("-> ", OUTPUT_FILE)
    plt.savefig(str(OUTPUT_FILE))


def main():
    data = parse_data_files()
    plot(data)


if __name__ == "__main__":
    main()
