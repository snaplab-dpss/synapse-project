#!/usr/bin/env python3

import os

from pathlib import Path

from utils.plot_config import *

from msgspec.json import decode
from msgspec import Struct

import matplotlib.pyplot as plt

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PLOTS_DIR = CURRENT_DIR / "plots"
OUTPUT_FNAME = "pcap_stats"
REPORTS_DIR = CURRENT_DIR / ".." / "tools" / "pcaps"

reports = [
    { "name": "caida_equinix_nyc", "file": "equinix-nyc.dirB.20190117-140000.UTC.anon.json" },
    { "name": "caida_chicago", "file": "equinix-chicago.json" },
    { "name": "univ1", "file": "univ1.json" },
    { "name": "univ2", "file": "univ2.json" },
]

class CDF(Struct):
    values: list[int]
    probabilities: list[float]

class StatsReport(Struct):
    start_utc_ns: int
    end_utc_ns: int
    total_pkts: int
    tcpudp_pkts: int
    pkt_bytes_avg: float
    pkt_bytes_stdev: float
    pkt_bytes_cdf: CDF
    total_flows: int
    total_symm_flows: int
    pkts_per_flow_avg: float
    pkts_per_flow_stdev: float
    pkts_per_flow_cdf: CDF
    top_k_flows_cdf: CDF
    top_k_flows_bytes_cdf: CDF
    flow_duration_us_avg: float
    flow_duration_us_stdev: float
    flow_duration_us_cdf: CDF
    flow_dts_us_avg: float
    flow_dts_us_stdev: float
    flow_dts_us_cdf: CDF

def parse_report(file: Path) -> StatsReport:
    print(f"Parsing report: {file}")
    assert file.exists()
    with open(file, "rb") as f:
        report = decode(f.read(), type=StatsReport)
        return report

def plot(name: str, report: StatsReport):
    figures: list[tuple[str, str, CDF]] = [
        ("pkt_bytes_cdf", "Packet bytes", report.pkt_bytes_cdf),
        ("pkts_per_flow_cdf", "Packets/flow", report.pkts_per_flow_cdf),
        ("top_k_flows_cdf", "Top-k flows", report.top_k_flows_cdf),
        ("top_k_flows_bytes_cdf", "Top-k flows (bytes)", report.top_k_flows_bytes_cdf),
        ("flow_duration_cdf", "Flow duration (us)", report.flow_duration_us_cdf),
        ("flow_ipt_cdf", "Flow inter-packet time (us)", report.flow_dts_us_cdf),
    ]

    for fname, xlabel, cdf in figures:
        fig_file_pdf = Path(PLOTS_DIR / f"{OUTPUT_FNAME}_{name}_{fname}.pdf")
        fig_file_png = Path(PLOTS_DIR / f"{OUTPUT_FNAME}_{name}_{fname}.png")

        fig, ax = plt.subplots()

        n_xticks = 6
        max_x = max(cdf.values)
        xtick_step = math.ceil(max_x / (n_xticks - 1))
        xticks = [ min(i * xtick_step, max_x) for i in range(n_xticks) ]
        yticks = [ i/10 for i in range(0, 11, 2) ]

        ax.set_xlabel(xlabel)
        ax.set_ylabel("CDF")
        ax.set_ylim(ymin=0, ymax=1.0)
        ax.set_yticks(yticks)

        if "top_k" in fname:
            ax.set_xscale("log")
            ax.set_xticks([1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7])
            ax.set_xlim(xmin=1e0, xmax=1.5e7)
        else:
            ax.set_xlim(xmin=0, xmax=max_x)
            ax.set_xticks(xticks)

        ax.plot([0] + cdf.values, [0] + cdf.probabilities, label=xlabel)

        fig.set_size_inches(width / 2, height / 2)
        fig.tight_layout(pad=0.1)

        print("-> ", fig_file_pdf)
        print("-> ", fig_file_png)

        plt.savefig(str(fig_file_pdf))
        plt.savefig(str(fig_file_png))

def main():
    for report in reports:
        report_file = REPORTS_DIR / report["file"]
        data = parse_report(report_file)
        plot(report["name"], data)

if __name__ == "__main__":
    main()
