#!/usr/bin/env python3

import os
import statistics

from utils.plot_config import *
from pathlib import Path

import matplotlib.pyplot as plt

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

DATA_FNAME = "tput_per_pkt_size_forwarder.csv"
OUTPUT_FNAME = "tput_per_pkt_size_forwarder"

def parse_data_file():
    raw_data = {}
    with open(DATA_DIR / DATA_FNAME, "r") as f:
        lines = f.readlines()
        for line in lines[1:]:
            parts = line.split(",")
            assert len(parts) == 4
            pkt_size = int(parts[1])
            tput_bps = float(parts[2])
            tput_pps = float(parts[3])

            if pkt_size not in raw_data:
                raw_data[pkt_size] = []
            raw_data[pkt_size].append((tput_bps, tput_pps))
    
    data = {}
    for pkt_size in raw_data.keys():
        if len(raw_data[pkt_size]) == 1:
            avg_tput_bps = raw_data[pkt_size][0][0]
            avg_tput_pps = raw_data[pkt_size][0][1]
            stdev_tput_bps = 0
            stdev_tput_pps = 0
        else:
            avg_tput_bps = statistics.mean([x[0] for x in raw_data[pkt_size]])
            avg_tput_pps = statistics.mean([x[1] for x in raw_data[pkt_size]])
            stdev_tput_bps = statistics.stdev([x[0] for x in raw_data[pkt_size]])
            stdev_tput_pps = statistics.stdev([x[1] for x in raw_data[pkt_size]])

        data[pkt_size] = (avg_tput_bps, stdev_tput_bps, avg_tput_pps, stdev_tput_pps)

    return data

def plot(data: dict):
    bar_width = 0.15

    fig_file_pdf = Path(PLOTS_DIR / f"{OUTPUT_FNAME}.pdf")

    fig, ax = plt.subplots()

    ax.set_ylim(ymin=0, ymax=3000)
    # ax.set_yscale("log")

    ax.set_ylabel("Throughput")
    ax.set_xlabel("Packet size (B)")

    ax.set_yticks(
        [100, 200, 400, 800, 1200, 2400, 3000 ],
        labels=["100G", "200G", "400G", "800G", "1.2T", "2.4T", "3T"]
    )

    pkt_sizes = [ str(pkt_size) for pkt_size in data.keys() ]
    tput_gbps = [ data[pkt_size][0] / 1e9 for pkt_size in data.keys() ]
    stdev_tput_gbps = [ data[pkt_size][1] / 1e9 for pkt_size in data.keys() ]

    ax.bar(pkt_sizes, tput_gbps)
    ax.errorbar(pkt_sizes, tput_gbps, yerr=stdev_tput_gbps, fmt='none', capsize=capsize, elinewidth=elinewidth, color="black")
    
    fig.set_size_inches(width / 2, height)
    fig.tight_layout(pad=0.1)

    print("-> ", fig_file_pdf)

    plt.savefig(str(fig_file_pdf))

def main():
    data = parse_data_file()
    plot(data)

if __name__ == "__main__":
    main()
