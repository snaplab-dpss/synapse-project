#!/usr/bin/env python3

import os
import statistics

from dataclasses import dataclass
from typing import Tuple
from pathlib import Path
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

from utils.plot_config import *
from utils.parser import parse_heatmap_data_file

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

TG_DATA_FILE = DATA_DIR / "tput_testbed_tg.csv"
TA_DATA_FILE = DATA_DIR / "tput_testbed_ta.csv"

TG_PPS_OUTPUT_FILE = PLOTS_DIR / "tput_testbed_tg_pps.pdf"
TA_PPS_OUTPUT_FILE = PLOTS_DIR / "tput_testbed_ta_pps.pdf"

TG_BPS_OUTPUT_FILE = PLOTS_DIR / "tput_testbed_tg_bps.pdf"
TA_BPS_OUTPUT_FILE = PLOTS_DIR / "tput_testbed_ta_bps.pdf"


@dataclass
class Data:
    requested_bps: Tuple[float, float]
    pktgen_bps: Tuple[float, float]
    pktgen_pps: Tuple[float, float]
    dut_ingress_bps: Tuple[float, float]
    dut_ingress_pps: Tuple[float, float]
    dut_egress_bps: Tuple[float, float]
    dut_egress_pps: Tuple[float, float]


def parse_data_file(file: Path) -> dict[int, Data]:
    raw_data = {}
    with open(file, "r") as f:
        lines = f.readlines()
        for line in lines[1:]:
            parts = line.split(",")
            assert len(parts) == 9
            pkt_size = int(parts[1])
            requested_bps = int(parts[2])
            pktgen_bps = int(parts[3])
            pktgen_pps = int(parts[4])
            dut_ingress_bps = int(parts[5])
            dut_ingress_pps = int(parts[6])
            dut_egress_bps = int(parts[7])
            dut_egress_pps = int(parts[8])

            if pkt_size not in raw_data:
                raw_data[pkt_size] = []
            raw_data[pkt_size].append(
                (
                    requested_bps,
                    pktgen_bps,
                    pktgen_pps,
                    dut_ingress_bps,
                    dut_ingress_pps,
                    dut_egress_bps,
                    dut_egress_pps,
                )
            )

    data = {}
    for pkt_size in raw_data.keys():
        if len(raw_data[pkt_size]) == 1:
            avg_requested_bps = raw_data[pkt_size][0][0]
            avg_pktgen_bps = raw_data[pkt_size][0][1]
            avg_pktgen_pps = raw_data[pkt_size][0][2]
            avg_dut_ingress_bps = raw_data[pkt_size][0][3]
            avg_dut_ingress_pps = raw_data[pkt_size][0][4]
            avg_dut_egress_bps = raw_data[pkt_size][0][5]
            avg_dut_egress_pps = raw_data[pkt_size][0][6]

            stdev_requested_bps = 0
            stdev_pktgen_bps = 0
            stdev_pktgen_pps = 0
            stdev_dut_ingress_bps = 0
            stdev_dut_ingress_pps = 0
            stdev_dut_egress_bps = 0
            stdev_dut_egress_pps = 0
        else:
            avg_requested_bps = statistics.mean([x[0] for x in raw_data[pkt_size]])
            avg_pktgen_bps = statistics.mean([x[1] for x in raw_data[pkt_size]])
            avg_pktgen_pps = statistics.mean([x[2] for x in raw_data[pkt_size]])
            avg_dut_ingress_bps = statistics.mean([x[3] for x in raw_data[pkt_size]])
            avg_dut_ingress_pps = statistics.mean([x[4] for x in raw_data[pkt_size]])
            avg_dut_egress_bps = statistics.mean([x[5] for x in raw_data[pkt_size]])
            avg_dut_egress_pps = statistics.mean([x[6] for x in raw_data[pkt_size]])

            stdev_requested_bps = statistics.stdev([x[0] for x in raw_data[pkt_size]])
            stdev_pktgen_bps = statistics.stdev([x[1] for x in raw_data[pkt_size]])
            stdev_pktgen_pps = statistics.stdev([x[2] for x in raw_data[pkt_size]])
            stdev_dut_ingress_bps = statistics.stdev([x[3] for x in raw_data[pkt_size]])
            stdev_dut_ingress_pps = statistics.stdev([x[4] for x in raw_data[pkt_size]])
            stdev_dut_egress_bps = statistics.stdev([x[5] for x in raw_data[pkt_size]])
            stdev_dut_egress_pps = statistics.stdev([x[6] for x in raw_data[pkt_size]])

        data[pkt_size] = Data(
            (avg_requested_bps, stdev_requested_bps),
            (avg_pktgen_bps, stdev_pktgen_bps),
            (avg_pktgen_pps, stdev_pktgen_pps),
            (avg_dut_ingress_bps, stdev_dut_ingress_bps),
            (avg_dut_ingress_pps, stdev_dut_ingress_pps),
            (avg_dut_egress_bps, stdev_dut_egress_bps),
            (avg_dut_egress_pps, stdev_dut_egress_pps),
        )

    return data


def plot_bps(data: dict[int, Data], file: Path):
    fig, ax = plt.subplots(constrained_layout=True)

    ax.set_ylim(ymin=0, ymax=3000)

    ax.set_ylabel("Throughput (Gbps)")
    ax.set_xlabel("Packet size (B)")

    ax.set_yticks([600, 1200, 1800, 2400, 3000])
    ax.yaxis.set_minor_locator(MultipleLocator(300))

    pkt_sizes = [str(pkt_size) for pkt_size in data.keys()]

    pktgen_gbps = [data[pkt_size].pktgen_bps[0] / 1e9 for pkt_size in data.keys()]
    stdev_pktgen_gbps = [data[pkt_size].pktgen_bps[1] / 1e9 for pkt_size in data.keys()]

    avg_tg_egress_gbps = [data[pkt_size].dut_ingress_bps[0] / 1e9 for pkt_size in data.keys()]
    stdev_tg_egress_gbps = [data[pkt_size].dut_ingress_bps[1] / 1e9 for pkt_size in data.keys()]

    data_to_show = {
        "Pktgen": (pktgen_gbps, stdev_pktgen_gbps),
        "Pktgen (30x)": ([30 * v for v in pktgen_gbps], [30 * v for v in stdev_pktgen_gbps]),
        "TG egress": (avg_tg_egress_gbps, stdev_tg_egress_gbps),
    }

    bar_width = 0.25
    ind = np.arange(len(pkt_sizes))
    pos = ind
    for attribute, (y, yerr) in data_to_show.items():
        rects = ax.bar(pos, y, bar_width, label=attribute)
        ax.errorbar(pos, y, yerr=yerr, fmt="none", capsize=capsize, markeredgewidth=markeredgewidth, elinewidth=elinewidth, color="black")
        ax.bar_label(rects, labels=[int(v) for v in y], padding=1, rotation=45, fontsize=6)
        pos = pos + bar_width

    ax.set_xticks(ind + (3.0 / 2) * bar_width, pkt_sizes)

    ax.legend(bbox_to_anchor=(0.45, 1.3), loc="upper center", ncols=5, columnspacing=0.5)
    fig.set_size_inches(width * 0.6, height * 0.6)

    fig_file_pdf = Path(file)
    print("-> ", fig_file_pdf)
    plt.savefig(str(fig_file_pdf))


def plot_pps(data: dict[int, Data], file: Path):
    fig, ax = plt.subplots(constrained_layout=True)

    ax.set_ylim(ymin=0, ymax=3500)

    ax.set_ylabel("Throughput (Mpps)")
    ax.set_xlabel("Packet size (B)")

    ax.set_yticks([700, 1400, 2100, 2800, 3500])
    ax.yaxis.set_minor_locator(MultipleLocator(350))

    pkt_sizes = [str(pkt_size) for pkt_size in data.keys()]

    pktgen_mpps = [data[pkt_size].pktgen_pps[0] / 1e6 for pkt_size in data.keys()]
    stdev_pktgen_mpps = [data[pkt_size].pktgen_pps[1] / 1e6 for pkt_size in data.keys()]

    avg_tg_egress_mpps = [data[pkt_size].dut_ingress_pps[0] / 1e6 for pkt_size in data.keys()]
    stdev_tg_egress_mpps = [data[pkt_size].dut_ingress_pps[1] / 1e6 for pkt_size in data.keys()]

    data_to_show = {
        "Pktgen": (pktgen_mpps, stdev_pktgen_mpps),
        "Pktgen (30x)": ([30 * v for v in pktgen_mpps], [30 * v for v in stdev_pktgen_mpps]),
        "TG egress": (avg_tg_egress_mpps, stdev_tg_egress_mpps),
    }

    bar_width = 0.25
    ind = np.arange(len(pkt_sizes))
    pos = ind
    for attribute, (y, yerr) in data_to_show.items():
        rects = ax.bar(pos, y, bar_width, label=attribute)
        ax.errorbar(pos, y, yerr=yerr, fmt="none", capsize=capsize, markeredgewidth=markeredgewidth, elinewidth=elinewidth, color="black")
        ax.bar_label(rects, labels=[int(v) for v in y], padding=1, rotation=45, fontsize=6)
        pos = pos + bar_width

    ax.set_xticks(ind + (3.0 / 2) * bar_width, pkt_sizes)

    ax.legend(bbox_to_anchor=(0.45, 1.3), loc="upper center", ncols=5, columnspacing=0.5)
    fig.set_size_inches(width * 0.6, height * 0.6)

    fig_file_pdf = Path(file)
    print("-> ", fig_file_pdf)
    plt.savefig(str(fig_file_pdf))


def main():
    tg_data = parse_data_file(TG_DATA_FILE)
    plot_bps(tg_data, TG_BPS_OUTPUT_FILE)
    plot_pps(tg_data, TG_PPS_OUTPUT_FILE)

    # ta_data = parse_data_file(TA_DATA_FILE)
    # plot_bps(ta_data, TA_BPS_OUTPUT_FILE)
    # plot_pps(ta_data, TA_PPS_OUTPUT_FILE)


if __name__ == "__main__":
    main()
