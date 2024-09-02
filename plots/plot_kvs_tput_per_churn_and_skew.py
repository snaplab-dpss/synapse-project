#!/usr/bin/env python3

import os

from pathlib import Path

from utils.plot_config import *

import matplotlib.pyplot as plt
import numpy as np

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

PLOTS_DIR = CURRENT_DIR / "plots"

OUTPUT_FNAME = "kvs_tput_per_churn_and_skew"

# Set INTERPOLATE to True to enable interpolation
INTERPOLATE = True

churn = [ "100M", "10M", "1M", "100K", "10K", "1K", "0" ]
topk = [ "0%", "5%", "10%", "15%", "20%", "25%" ]

data_per_nf = {
    "netcache": np.array([
        [0, 0.01, 0.01, 0.01, 0.01, 0.01], # 100M
        [0, 0.1, 0.1, 0.1, 0.1, 0.1], # 10M
        [0, 1, 1, 1, 1, 1], # 1M
        [10, 10, 10, 10, 10, 10], # 100K
        [100, 100, 100, 100, 100, 100], # 10K
        [100, 100, 100, 100, 100, 100], # 1K
        [100, 100, 100, 100, 100, 100], # 0K
    ]),
    "switcharoo": np.array([
        [100, 80, 60, 40, 20, 10], # 100M
        [100, 90, 80, 60, 40, 20], # 10M
        [100, 90, 80, 60, 45, 40], # 1M
        [100, 90, 80, 60, 55, 50], # 100K
        [100, 90, 80, 60, 55, 50], # 10K
        [100, 90, 80, 60, 55, 50], # 1K
        [100, 100, 100, 100, 100, 100], # 0K
    ]),
    "synapse": np.array([
        [100, 90, 80, 70, 70, 80], # 100M
        [100, 90, 80, 70, 70, 80], # 10M
        [100, 90, 80, 70, 70, 80], # 1M
        [100, 90, 80, 70, 70, 80], # 100K
        [100, 90, 80, 70, 70, 80], # 10K
        [100, 90, 80, 70, 70, 80], # 1K
        [100, 100, 100, 100, 100, 100], # 0K
    ]),
}

def discrete(data, ax):
    ax.grid(False)
    ax.spines[:].set_visible(False)
    ax.set_xticks(np.arange(data.shape[1]+1)-.5, minor=True)
    ax.set_yticks(np.arange(data.shape[0]+1)-.5, minor=True)
    ax.grid(which="minor", color="w", linestyle='-', linewidth=1)
    ax.tick_params(which="minor", bottom=False, left=False)

def plot(data_per_nf: dict):
    for nf in data_per_nf.keys():
        data = data_per_nf[nf]

        fig_file_pdf = Path(PLOTS_DIR / f"{OUTPUT_FNAME}_{nf}.pdf")

        # Nice colormaps:
        # https://matplotlib.org/stable/tutorials/colors/colormaps.html
        # - plasma
        # - viridis

        fig, ax = plt.subplots()
        im = ax.imshow(data, vmin=0, vmax=100, cmap="plasma",
                       interpolation="spline36" if INTERPOLATE else "")
        
        # Create colorbar
        assert ax.figure
        cbar = ax.figure.colorbar(im, ax=ax)
        cbar.ax.set_ylabel("Tput (Gbps)", rotation=-90, va="bottom")

        # Show all ticks and label them with the respective list entries
        ax.set_xticks(np.arange(len(topk)), labels=topk)
        ax.set_yticks(np.arange(len(churn)), labels=churn)

        ax.set_xlabel("Top-k flows (\\%)")
        ax.set_ylabel("Churn (fpm)")

        # Rotate the tick labels and set their alignment.
        plt.setp(ax.get_xticklabels(), rotation=45, ha="right", rotation_mode="anchor")

        ax.grid(False)
        
        if not INTERPOLATE:
            ax.spines[:].set_visible(False)
            ax.set_xticks(np.arange(data.shape[1]+1)-.5, minor=True)
            ax.set_yticks(np.arange(data.shape[0]+1)-.5, minor=True)
            ax.grid(which="minor", color="w", linestyle='-', linewidth=1)
            ax.tick_params(which="minor", bottom=False, left=False)

        fig.set_size_inches(width * 0.5, height * 0.8)
        fig.tight_layout()

        print("-> ", fig_file_pdf)
        plt.savefig(str(fig_file_pdf))

def main():
    plot(data_per_nf)

if __name__ == "__main__":
    main()
