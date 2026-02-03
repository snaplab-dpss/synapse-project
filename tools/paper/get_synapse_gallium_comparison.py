#!/usr/bin/env python3

import os

from pathlib import Path
from statistics import mean

from tput_eval_data import parse_tput_eval_data_file

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / ".." / "..").resolve()
DATA_DIR = PROJECT_DIR / "eval" / "data"

DATA = [
    {
        "nf": "KVS",
        "synapse_data_file": DATA_DIR / "tput_synapse_kvs.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_kvs.csv",
    },
    {
        "nf": "Firewall",
        "synapse_data_file": DATA_DIR / "tput_synapse_fw.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_fw.csv",
    },
    {
        "nf": "NAT",
        "synapse_data_file": DATA_DIR / "tput_synapse_nat.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_nat.csv",
    },
    {
        "nf": "PSD",
        "synapse_data_file": DATA_DIR / "tput_synapse_psd.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_psd.csv",
    },
    {
        "nf": "CL",
        "synapse_data_file": DATA_DIR / "tput_synapse_cl.csv",
        "gallium_data_file": DATA_DIR / "tput_gallium_cl.csv",
    },
]


def main():
    for nf_data in DATA:
        synapse_data = parse_tput_eval_data_file(nf_data["synapse_data_file"])
        gallium_data = parse_tput_eval_data_file(nf_data["gallium_data_file"])

        synapse_keys = set(key for key in synapse_data.keys())
        gallium_keys = set(key for key in gallium_data.keys())
        common_keys = synapse_keys.intersection(gallium_keys)

        synapse_avg_pps = int(mean([value.get_avg_pps()[0] for key, value in synapse_data.items() if key in common_keys]))
        gallium_avg_pps = int(mean([value.get_avg_pps()[0] for key, value in gallium_data.items() if key in common_keys]))

        print(f"=== NF: {nf_data['nf']} ===")
        print(f"Synapse: {synapse_avg_pps} pps")
        print(f"Gallium: {gallium_avg_pps} pps")
        if gallium_avg_pps == 0:
            print("Speedup: N/A (Gallium avg pps is 0)")
        else:
            print(f"Speedup: {synapse_avg_pps / gallium_avg_pps:.2f}x")


if __name__ == "__main__":
    main()
