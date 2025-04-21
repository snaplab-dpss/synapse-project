#!/usr/bin/env python3

import os

from pathlib import Path

from utils.heatmap import *

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

DATA_FILE = DATA_DIR / "tput_netcache.csv"

BPS_OUTPUT_FILE = PLOTS_DIR / "tput_netcache_bps.pdf"
PPS_OUTPUT_FILE = PLOTS_DIR / "tput_netcache_pps.pdf"
BPS_SCATTER_OUTPUT_FILE = PLOTS_DIR / "tput_netcache_bps_scatter.pdf"
PPS_SCATTER_OUTPUT_FILE = PLOTS_DIR / "tput_netcache_pps_scatter.pdf"
HEATMAP_OUTPUT_FILE = PLOTS_DIR / "tput_netcache_heatmap.pdf"


def main():
    heatmap_data = parse_heatmap_data_file(DATA_FILE)
    plot_bps(heatmap_data, BPS_OUTPUT_FILE)
    plot_pps(heatmap_data, PPS_OUTPUT_FILE)
    plot_bps_scatter(heatmap_data, BPS_SCATTER_OUTPUT_FILE)
    # plot_heatmap(data, HEATMAP_OUTPUT_FILE)
    plot_heatmap_v2(heatmap_data, HEATMAP_OUTPUT_FILE)


if __name__ == "__main__":
    main()
