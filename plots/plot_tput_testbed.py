#!/usr/bin/env python3

import os
import statistics

from utils.plot_config import *
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

TG_DATA_FILE = DATA_DIR / "tput_testbed_tg.csv"
TA_DATA_FILE = DATA_DIR / "tput_testbed_ta.csv"

TG_PPS_OUTPUT_FILE = PLOTS_DIR / "tput_testbed_tg_pps.pdf"
TA_PPS_OUTPUT_FILE = PLOTS_DIR / "tput_testbed_ta_pps.pdf"

TG_BPS_OUTPUT_FILE = PLOTS_DIR / "tput_testbed_tg_bps.pdf"
TA_BPS_OUTPUT_FILE = PLOTS_DIR / "tput_testbed_ta_bps.pdf"


def parse_data_file(file: Path):
    raw_data = {}
    with open(file, "r") as f:
        lines = f.readlines()
        for line in lines[1:]:
            parts = line.split(",")
            assert len(parts) == 7
            pkt_size = int(parts[1])
            requested_bps = int(parts[2])
            pktgen_tput_bps = int(parts[3])
            pktgen_tput_pps = int(parts[4])
            tput_bps = int(parts[5])
            tput_pps = int(parts[6])

            if pkt_size not in raw_data:
                raw_data[pkt_size] = []
            raw_data[pkt_size].append((requested_bps, pktgen_tput_bps, pktgen_tput_pps, tput_bps, tput_pps))

    data = {}
    for pkt_size in raw_data.keys():
        if len(raw_data[pkt_size]) == 1:
            avg_requested_bps = raw_data[pkt_size][0][0]
            avg_pktgen_tput_bps = raw_data[pkt_size][0][1]
            avg_pktgen_tput_pps = raw_data[pkt_size][0][2]
            avg_tput_bps = raw_data[pkt_size][0][3]
            avg_tput_pps = raw_data[pkt_size][0][4]

            stdev_requested_bps = 0
            stdev_pktgen_tput_bps = 0
            stdev_pktgen_tput_pps = 0
            stdev_tput_bps = 0
            stdev_tput_pps = 0
        else:
            avg_requested_bps = statistics.mean([x[0] for x in raw_data[pkt_size]])
            avg_pktgen_tput_bps = statistics.mean([x[1] for x in raw_data[pkt_size]])
            avg_pktgen_tput_pps = statistics.mean([x[2] for x in raw_data[pkt_size]])
            avg_tput_bps = statistics.mean([x[3] for x in raw_data[pkt_size]])
            avg_tput_pps = statistics.mean([x[4] for x in raw_data[pkt_size]])

            stdev_requested_bps = statistics.stdev([x[0] for x in raw_data[pkt_size]])
            stdev_pktgen_tput_bps = statistics.stdev([x[1] for x in raw_data[pkt_size]])
            stdev_pktgen_tput_pps = statistics.stdev([x[2] for x in raw_data[pkt_size]])
            stdev_tput_bps = statistics.stdev([x[3] for x in raw_data[pkt_size]])
            stdev_tput_pps = statistics.stdev([x[4] for x in raw_data[pkt_size]])

        data[pkt_size] = (
            (avg_requested_bps, stdev_requested_bps),
            (avg_pktgen_tput_bps, stdev_pktgen_tput_bps),
            (avg_pktgen_tput_pps, stdev_pktgen_tput_pps),
            (avg_tput_bps, stdev_tput_bps),
            (avg_tput_pps, stdev_tput_pps),
        )

    return data


def plot_bps(data: dict, file: Path):
    fig, ax = plt.subplots()

    ax.set_ylim(ymin=0, ymax=3000)

    ax.set_ylabel("Throughput (bps)")
    ax.set_xlabel("Packet size (B)")

    ax.set_yticks([600, 1200, 1800, 2400, 3000], labels=["600G", "1.2T", "1.8T", "2.4T", "3T"])
    ax.yaxis.set_minor_locator(MultipleLocator(300))

    pkt_sizes = [str(pkt_size) for pkt_size in data.keys()]

    # requested_gbps = [data[pkt_size][0][0] / 1e9 for pkt_size in data.keys()]
    # stdev_requested_gbps = [data[pkt_size][0][1] / 1e9 for pkt_size in data.keys()]

    pktgen_gbps = [data[pkt_size][1][0] / 1e9 for pkt_size in data.keys()]
    stdev_pktgen_gbps = [data[pkt_size][1][1] / 1e9 for pkt_size in data.keys()]

    # pktgen_pps = [data[pkt_size][2][0] for pkt_size in data.keys()]
    # stdev_pktgen_pps = [data[pkt_size][2][1] for pkt_size in data.keys()]

    tput_gbps = [data[pkt_size][3][0] / 1e9 for pkt_size in data.keys()]
    stdev_tput_gbps = [data[pkt_size][3][1] / 1e9 for pkt_size in data.keys()]

    # tput_pps = [data[pkt_size][4][0] for pkt_size in data.keys()]
    # stdev_tput_pps = [data[pkt_size][4][1] for pkt_size in data.keys()]

    data_to_show = {
        "Pktgen (Gbps)": (pktgen_gbps, stdev_pktgen_gbps),
        "Pktgen (30xGbps)": ([30 * v for v in pktgen_gbps], [30 * v for v in stdev_pktgen_gbps]),
        "Tput (Gbps)": (tput_gbps, stdev_tput_gbps),
    }

    bar_width = 0.25
    ind = np.arange(len(pkt_sizes))
    pos = ind
    for attribute, (y, yerr) in data_to_show.items():
        rects = ax.bar(pos, y, bar_width, label=attribute)
        ax.errorbar(pos, y, yerr=yerr, fmt="none", capsize=capsize, markeredgewidth=markeredgewidth, elinewidth=elinewidth, color="black")
        ax.bar_label(rects, labels=[int(v) for v in y], padding=1, rotation=45, fontsize=3)
        pos = pos + bar_width

    ax.set_xticks(ind + (3.0 / 2) * bar_width, pkt_sizes)

    ax.legend(bbox_to_anchor=(0.45, 1.1), loc="upper center", ncols=5, columnspacing=0.5, prop={"size": 5})
    fig.set_size_inches(width * 0.6, height)
    fig.tight_layout(pad=0.1)

    fig_file_pdf = Path(file)
    print("-> ", fig_file_pdf)
    plt.savefig(str(fig_file_pdf))


def plot_pps(data: dict, file: Path):
    fig, ax = plt.subplots()

    ax.set_ylim(ymin=0, ymax=3000)

    ax.set_ylabel("Throughput (pps)")
    ax.set_xlabel("Packet size (B)")

    ax.set_yticks([500, 1000, 1500, 2000, 2500, 3000, 3500], labels=["500M", "1G", "1.5G", "2G", "2.5G", "3G", "3.5G"])
    ax.yaxis.set_minor_locator(MultipleLocator(250))

    pkt_sizes = [str(pkt_size) for pkt_size in data.keys()]

    # requested_gbps = [data[pkt_size][0][0] / 1e9 for pkt_size in data.keys()]
    # stdev_requested_gbps = [data[pkt_size][0][1] / 1e9 for pkt_size in data.keys()]

    # pktgen_gbps = [data[pkt_size][1][0] / 1e9 for pkt_size in data.keys()]
    # stdev_pktgen_gbps = [data[pkt_size][1][1] / 1e9 for pkt_size in data.keys()]

    pktgen_mpps = [data[pkt_size][2][0] / 1e6 for pkt_size in data.keys()]
    stdev_pktgen_mpps = [data[pkt_size][2][1] / 1e6 for pkt_size in data.keys()]

    # tput_gbps = [data[pkt_size][3][0] / 1e9 for pkt_size in data.keys()]
    # stdev_tput_gbps = [data[pkt_size][3][1] / 1e9 for pkt_size in data.keys()]

    tput_mpps = [data[pkt_size][4][0] / 1e6 for pkt_size in data.keys()]
    stdev_tput_mpps = [data[pkt_size][4][1] / 1e6 for pkt_size in data.keys()]

    data_to_show = {
        "Pktgen (Gpps)": (pktgen_mpps, stdev_pktgen_mpps),
        "Pktgen (30xGpps)": ([30 * v for v in pktgen_mpps], [30 * v for v in stdev_pktgen_mpps]),
        "Tput (Gpps)": (tput_mpps, stdev_tput_mpps),
    }

    bar_width = 0.25
    ind = np.arange(len(pkt_sizes))
    pos = ind
    for attribute, (y, yerr) in data_to_show.items():
        rects = ax.bar(pos, y, bar_width, label=attribute)
        ax.errorbar(pos, y, yerr=yerr, fmt="none", capsize=capsize, markeredgewidth=markeredgewidth, elinewidth=elinewidth, color="black")
        ax.bar_label(rects, labels=[int(v) for v in y], padding=1, rotation=45, fontsize=3)
        pos = pos + bar_width

    ax.set_xticks(ind + (3.0 / 2) * bar_width, pkt_sizes)

    ax.legend(bbox_to_anchor=(0.45, 1.1), loc="upper center", ncols=5, columnspacing=0.5, prop={"size": 5})
    fig.set_size_inches(width * 0.6, height)
    fig.tight_layout(pad=0.1)

    fig_file_pdf = Path(file)
    print("-> ", fig_file_pdf)
    plt.savefig(str(fig_file_pdf))


def main():
    tg_data = parse_data_file(TG_DATA_FILE)
    plot_bps(tg_data, TG_BPS_OUTPUT_FILE)
    plot_pps(tg_data, TG_PPS_OUTPUT_FILE)

    ta_data = parse_data_file(TA_DATA_FILE)
    plot_bps(ta_data, TA_BPS_OUTPUT_FILE)
    plot_pps(ta_data, TA_PPS_OUTPUT_FILE)


if __name__ == "__main__":
    main()
