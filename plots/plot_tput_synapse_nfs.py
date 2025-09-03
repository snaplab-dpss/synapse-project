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
    #     "title": "KVS HHTable",
    #     "data_file": DATA_DIR / "tput_synapse_kvs_hhtable.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_kvs_hhtable_heatmap.pdf",
    # },
    # {
    #     "title": "KVS GuardedMapTable",
    #     "data_file": DATA_DIR / "tput_synapse_kvs_guardedmaptable.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_kvs_guardedmaptable_heatmap.pdf",
    # },
    # {
    #     "title": "KVS MapTable",
    #     "data_file": DATA_DIR / "tput_synapse_kvs_maptable.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_kvs_maptable_heatmap.pdf",
    # },
    {
        "title": "KVS Cuckoo",
        "data_file": DATA_DIR / "tput_synapse_kvs_cuckoo.csv",
        "bps_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_bps.pdf",
        "pps_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_pps.pdf",
        "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_bps_scatter.pdf",
        "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_pps_scatter.pdf",
        "heatmap_output_file": PLOTS_DIR / "tput_synapse_kvs_cuckoo_heatmap.pdf",
    },
    # {
    #     "title": "Firewall",
    #     "data_file": DATA_DIR / "tput_synapse_fw.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_fw_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_fw_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_fw_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_fw_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_fw_heatmap.pdf",
    # },
    # {
    #     "title": "NAT",
    #     "data_file": DATA_DIR / "tput_synapse_nat.csv",
    #     "bps_output_file": PLOTS_DIR / "tput_synapse_nat_bps.pdf",
    #     "pps_output_file": PLOTS_DIR / "tput_synapse_nat_pps.pdf",
    #     "bps_scatter_output_file": PLOTS_DIR / "tput_synapse_nat_bps_scatter.pdf",
    #     "pps_scatter_output_file": PLOTS_DIR / "tput_synapse_nat_pps_scatter.pdf",
    #     "heatmap_output_file": PLOTS_DIR / "tput_synapse_nat_heatmap.pdf",
    # },
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
