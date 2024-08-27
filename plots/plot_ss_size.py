#!/usr/bin/env python3

import os

from pathlib import Path

from utils.plot_config import *

import matplotlib.pyplot as plt

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

PLOTS_DIR = CURRENT_DIR / "plots"

COMPONENT_BASE = "Tables+Registers"
COMPONENT_FCFS_CACHED_TABLE = "FCFS Cached Table"
COMPONENT_HH_CACHED_TABLE = "HH Cached Table"
COMPONENT_RECIRCULATIONS = "Recirculations"
COMPONENT_BDD_REORDERING = "BDD reordering"

OUTPUT_FNAME = "ss_size"

nfs = [ "FW", "NAT", "KVS", "LB" ]

data = {
    COMPONENT_BASE: [ 1e4, 0, 0, 0 ],
    COMPONENT_FCFS_CACHED_TABLE: [ 1e5, 0, 0, 0 ],
    # COMPONENT_HH_CACHED_TABLE: [ 0, 0, 0, 0 ],
    COMPONENT_RECIRCULATIONS: [ 1e8, 0, 0, 0 ],
    COMPONENT_BDD_REORDERING: [ 1e14, 0, 0, 0 ],
}

def plot_ss_size():
    fig_file_pdf = Path(PLOTS_DIR / f"{OUTPUT_FNAME}.pdf")
    fig_file_png = Path(PLOTS_DIR / f"{OUTPUT_FNAME}.png")

    fig, ax = plt.subplots()
    bottom = np.zeros(len(nfs))

    ax.set_ylim(ymin=1e0, ymax=1e15)
    ax.set_yscale("log")
    ax.set_ylabel("Search space size")

    for (component, ss_size), hatch in zip(data.items(), itertools.cycle(hatch_list)):
        ax.bar(nfs, ss_size, 0.6, label=component, bottom=bottom, alpha=.99, hatch=hatch)
        bottom = ss_size

    ax.legend(bbox_to_anchor=(0.4, 1.3), loc='upper center', ncols=2)

    fig.set_size_inches(width / 2, height)
    fig.tight_layout(pad=0.1)

    print("-> ", fig_file_pdf)
    print("-> ", fig_file_png)

    plt.savefig(str(fig_file_pdf))
    plt.savefig(str(fig_file_png))

def main():
    plot_ss_size()

if __name__ == "__main__":
    main()
