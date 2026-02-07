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
        "nf": "FW",
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
    avgs = []
    mins = []
    maxs = []

    for nf_data in DATA:
        synapse_data = parse_tput_eval_data_file(nf_data["synapse_data_file"])
        gallium_data = parse_tput_eval_data_file(nf_data["gallium_data_file"])

        synapse_keys = set(key for key in synapse_data.keys())
        gallium_keys = set(key for key in gallium_data.keys())
        common_keys = synapse_keys.intersection(gallium_keys)

        synapse_avg_pps = int(mean([value.get_avg_pps()[0] for key, value in synapse_data.items() if key in common_keys]))
        gallium_avg_pps = int(mean([value.get_avg_pps()[0] for key, value in gallium_data.items() if key in common_keys]))

        speedups = []
        for key in common_keys:
            synapse_avg_pps = synapse_data[key].get_avg_pps()[0]
            gallium_avg_pps = gallium_data[key].get_avg_pps()[0]

            if gallium_avg_pps == 0:
                continue

            speedup = synapse_avg_pps / gallium_avg_pps
            speedups.append(speedup)

        if not speedups:
            print(f"{nf_data['nf']:<8}: No common data points to compare.")
            continue

        avg_speedup = mean(speedups)
        min_speedup = min(speedups)
        max_speedup = max(speedups)

        avgs.append(avg_speedup)
        mins.append(min_speedup)
        maxs.append(max_speedup)

        print(f"{nf_data['nf']:<8}: Avg Speedup = {avg_speedup:7.2f}x, Min Speedup = {min_speedup:7.2f}x, Max Speedup = {max_speedup:7.2f}x")

    overall_avg_speedup = mean(avgs)
    overall_min_speedup = min(mins)
    overall_max_speedup = max(maxs)

    print(f"{'Overall':<8}: Avg Speedup = {overall_avg_speedup:7.2f}x, Min Speedup = {overall_min_speedup:7.2f}x, Max Speedup = {overall_max_speedup:7.2f}x")


if __name__ == "__main__":
    main()
