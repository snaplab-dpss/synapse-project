#!/usr/bin/env python3

import os
import statistics
from typing import Tuple

from utils.plot_config import *
from dataclasses import dataclass
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

DATA_FILE = DATA_DIR / "tput_kvs_server.csv"
BPS_OUTPUT_FILE = PLOTS_DIR / "tput_kvs_server_bps.pdf"
PPS_OUTPUT_FILE = PLOTS_DIR / "tput_kvs_server_pps.pdf"


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
        for line in f.readlines():
            if line.startswith("#"):
                continue

            parts = line.split(",")
            assert len(parts) == 9
            delay_ns = int(parts[1])
            requested_bps = int(parts[2])
            pktgen_bps = int(parts[3])
            pktgen_pps = int(parts[4])
            dut_ingress_bps = int(parts[5])
            dut_ingress_pps = int(parts[6])
            dut_egress_bps = int(parts[7])
            dut_egress_pps = int(parts[8])

            if delay_ns not in raw_data:
                raw_data[delay_ns] = []
            raw_data[delay_ns].append(
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
    for delay_ns in raw_data.keys():
        if len(raw_data[delay_ns]) == 1:
            avg_requested_bps = raw_data[delay_ns][0][0]
            avg_pktgen_bps = raw_data[delay_ns][0][1]
            avg_pktgen_pps = raw_data[delay_ns][0][2]
            avg_dut_ingress_bps = raw_data[delay_ns][0][3]
            avg_dut_ingress_pps = raw_data[delay_ns][0][4]
            avg_dut_egress_bps = raw_data[delay_ns][0][5]
            avg_dut_egress_pps = raw_data[delay_ns][0][6]

            stdev_requested_bps = 0
            stdev_pktgen_bps = 0
            stdev_pktgen_pps = 0
            stdev_dut_ingress_bps = 0
            stdev_dut_ingress_pps = 0
            stdev_dut_egress_bps = 0
            stdev_dut_egress_pps = 0
        else:
            avg_requested_bps = statistics.mean([x[0] for x in raw_data[delay_ns]])
            avg_pktgen_bps = statistics.mean([x[1] for x in raw_data[delay_ns]])
            avg_pktgen_pps = statistics.mean([x[2] for x in raw_data[delay_ns]])
            avg_dut_ingress_bps = statistics.mean([x[3] for x in raw_data[delay_ns]])
            avg_dut_ingress_pps = statistics.mean([x[4] for x in raw_data[delay_ns]])
            avg_dut_egress_bps = statistics.mean([x[5] for x in raw_data[delay_ns]])
            avg_dut_egress_pps = statistics.mean([x[6] for x in raw_data[delay_ns]])

            stdev_requested_bps = statistics.stdev([x[0] for x in raw_data[delay_ns]])
            stdev_pktgen_bps = statistics.stdev([x[1] for x in raw_data[delay_ns]])
            stdev_pktgen_pps = statistics.stdev([x[2] for x in raw_data[delay_ns]])
            stdev_dut_ingress_bps = statistics.stdev([x[3] for x in raw_data[delay_ns]])
            stdev_dut_ingress_pps = statistics.stdev([x[4] for x in raw_data[delay_ns]])
            stdev_dut_egress_bps = statistics.stdev([x[5] for x in raw_data[delay_ns]])
            stdev_dut_egress_pps = statistics.stdev([x[6] for x in raw_data[delay_ns]])

        data[delay_ns] = Data(
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
    fig, ax = plt.subplots()

    ax.set_ylim(ymin=0, ymax=100)

    ax.set_ylabel("Throughput (bps)")
    ax.set_xlabel("Delay (ns)")

    ax.set_yticks([0, 20, 40, 60, 80, 100], labels=["0", "20G", "40G", "60G", "80G", "100G"])
    ax.yaxis.set_minor_locator(MultipleLocator(10))

    delay_nss = [str(delay_ns) for delay_ns in data.keys()]

    avg_tput_gbps = [data[delay_ns].dut_egress_bps[0] / 1e9 for delay_ns in data.keys()]
    stdev_tput_gbps = [data[delay_ns].dut_egress_bps[1] / 1e9 for delay_ns in data.keys()]

    ax.errorbar(delay_nss, avg_tput_gbps, yerr=stdev_tput_gbps, fmt="none", capsize=capsize, markeredgewidth=markeredgewidth, elinewidth=elinewidth, color="black")
    ax.bar(delay_nss, avg_tput_gbps)

    fig.set_size_inches(width * 0.6, height)
    fig.tight_layout(pad=0.1)

    fig_file_pdf = Path(file)
    print("-> ", fig_file_pdf)
    plt.savefig(str(fig_file_pdf))


def plot_pps(data: dict[int, Data], file: Path):
    fig, ax = plt.subplots()

    ax.set_ylim(ymin=0, ymax=60)

    ax.set_ylabel("Throughput (pps)")
    ax.set_xlabel("Delay (ns)")

    ax.set_yticks([0, 20, 40, 60], labels=["0", "20M", "40M", "60M"])
    ax.yaxis.set_minor_locator(MultipleLocator(10))

    delay_nss = [str(delay_ns) for delay_ns in data.keys()]

    avg_tput_gbps = [data[delay_ns].dut_egress_pps[0] / 1e6 for delay_ns in data.keys()]
    stdev_tput_gbps = [data[delay_ns].dut_egress_pps[1] / 1e6 for delay_ns in data.keys()]

    ax.errorbar(delay_nss, avg_tput_gbps, yerr=stdev_tput_gbps, fmt="none", capsize=capsize, markeredgewidth=markeredgewidth, elinewidth=elinewidth, color="black")
    ax.bar(delay_nss, avg_tput_gbps)

    fig.set_size_inches(width * 0.6, height)
    fig.tight_layout(pad=0.1)

    fig_file_pdf = Path(file)
    print("-> ", fig_file_pdf)
    plt.savefig(str(fig_file_pdf))


def main():
    data = parse_data_file(DATA_FILE)
    plot_bps(data, BPS_OUTPUT_FILE)
    plot_pps(data, PPS_OUTPUT_FILE)


if __name__ == "__main__":
    main()
