#!/usr/bin/env python3

import os

from pathlib import Path

from utils.plot_config import *

import matplotlib.pyplot as plt

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

PLOTS_DIR = CURRENT_DIR / "plots"

OUTPUT_FNAME = "motivation_throughput"

nf = "KVS"
solutions = [ "NetCache", "Switcharoo", "Synapse" ]
workloads = [ "High skew\nLow churn", "Low skew\nHigh churn", "Medium skew\nMedium churn",]

data = {
    "NetCache": {
        "y": [100e9, 10e9, 10e9],
        "yerr": [0, 0, 0],
    },
    "Switcharoo": {
        "y": [10e9, 100e9, 10e9],
        "yerr": [0, 0, 0],
    },
    "Synapse": {
        "y": [100e9, 100e9, 70e9],
        "yerr": [0, 0, 0],
    },
}

def plot(data: dict):
    ind = np.arange(len(workloads))
    bar_width = 0.15

    fig_file_pdf = Path(PLOTS_DIR / f"{OUTPUT_FNAME}_{nf}.pdf")

    fig, ax = plt.subplots()

    ax.set_ylim(ymin=0, ymax=100e9)
    ax.set_ylabel("Tput (Gbps)")

    ax.set_yticks(np.arange(0, 101e9, 20e9), labels=np.arange(0, 101, 20))
    ax.set_yticks(np.arange(10e9, 101e9, 20e9), minor=True)

    colors = [ '#2400D8', '#3D87FF', '#99EAFF', '#FF3D3D', ]
    pos = ind
    for (sol, throughput_per_workload), hatch, color in zip(data.items(), itertools.cycle(hatch_list), itertools.cycle(colors)):
        y = throughput_per_workload['y']
        yerr = throughput_per_workload['yerr']
        ax.bar(pos, y, bar_width, yerr=yerr,
                label=sol, alpha=.99, hatch=hatch,
                error_kw=dict(lw=1, capsize=1, capthick=0.3), color=color)
        pos = pos + bar_width

    ax.set_xticks(ind + (3.0/2)*bar_width, workloads)

    ax.legend(bbox_to_anchor=(0.5, 1.4), loc='upper center', ncols=3)
    # ax.legend(loc='best')
    fig.set_size_inches(width / 2, height * 0.5)
    fig.tight_layout(pad=0.1)

    print("-> ", fig_file_pdf)

    plt.savefig(str(fig_file_pdf))

def main():
    plot(data)

if __name__ == "__main__":
    main()
