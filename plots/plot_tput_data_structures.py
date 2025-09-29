#!/usr/bin/env python3

import os

from pathlib import Path

from utils.heatmap import *
from utils.parser import parse_heatmap_data_file


CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

NFS = [
    # {
    #     "title": "MapTable",
    #     "data_file": DATA_DIR / "tput_map_table.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_map_table_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_map_table_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_map_table_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_map_table_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_map_table_heatmap.pdf",
    # },
    # {
    #     "title": "CuckooHashTable",
    #     "data_file": DATA_DIR / "tput_cuckoo_hash_table.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_cuckoo_hash_table_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_cuckoo_hash_table_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_cuckoo_hash_table_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_cuckoo_hash_table_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_cuckoo_hash_table_heatmap.pdf",
    # },
    *[
        {
            "title": f"FCFSCachedTable {cache_capacity}",
            "data_file": DATA_DIR / f"tput_fcfs_cached_table_{cache_capacity}.csv",
            "bps_output_file": PLOTS_DIR / f"tput_fcfs_cached_table_{cache_capacity}_bps.pdf",
            "pps_output_file": PLOTS_DIR / f"tput_fcfs_cached_table_{cache_capacity}_pps.pdf",
            "bps_scatter_output_file": PLOTS_DIR / f"tput_fcfs_cached_table_{cache_capacity}_bps_scatter.pdf",
            "pps_scatter_output_file": PLOTS_DIR / f"tput_fcfs_cached_table_{cache_capacity}_pps_scatter.pdf",
            "heatmap_output_file": PLOTS_DIR / f"tput_fcfs_cached_table_{cache_capacity}_heatmap.pdf",
        }
        for cache_capacity in [256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536]
    ],
]


def main():
    for nf in NFS:
        DATA_FILE = nf["data_file"]
        BPS_OUTPUT_FILE = nf["bps_output_file"]
        PPS_OUTPUT_FILE = nf["pps_output_file"]
        BPS_SCATTER_OUTPUT_FILE = nf["bps_scatter_output_file"]
        HEATMAP_OUTPUT_FILE = nf["heatmap_output_file"]

        heatmap_data = parse_heatmap_data_file(DATA_FILE)
        plot_bps(heatmap_data, BPS_OUTPUT_FILE)
        plot_pps(heatmap_data, PPS_OUTPUT_FILE)
        plot_bps_scatter(heatmap_data, BPS_SCATTER_OUTPUT_FILE)
        plot_heatmap(heatmap_data, HEATMAP_OUTPUT_FILE)


if __name__ == "__main__":
    main()
