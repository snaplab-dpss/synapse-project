#!/usr/bin/env python3

from utils.heatmap import Key, HeatmapData, plot_heatmap_v2
from utils.values import Values
from utils.synapse_report import *

from pathlib import Path
from itertools import product

import os

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__))).resolve()
PROJECT_DIR = CURRENT_DIR.parent

PLOTS_DIR = PROJECT_DIR / "plots" / "plots"
DATA_DIR = PROJECT_DIR / "synthesized"

NFS = ["kvs"]
TOTAL_FLOWS = [25_000]
CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
HEURISTICS = ["max-tput", "ds-pref-simple", "ds-pref-guardedmaptable", "ds-pref-hhtable", "ds-pref-cuckoo"]


def get_nf_name(
    nf: str,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
    heuristic: str,
) -> str:
    name = f"{nf}-f{total_flows}-c{churn_fpm}-{'unif' if zipf_param == 0.0 else 'zipf'}"
    if zipf_param != 0.0:
        s = str(int(zipf_param) if int(zipf_param) == zipf_param else zipf_param).replace(".", "_")
        name += s
    name += f"-h{heuristic}"
    return name


def main():
    combinations = product(NFS, HEURISTICS)

    for nf, heuristic in combinations:
        traffic_combinations = product(TOTAL_FLOWS, CHURN_FPM, ZIPF_PARAMS)

        heatmap_data = HeatmapData()

        found_all = True
        for total_flows, churn_fpm, zipf_param in traffic_combinations:
            nf_name = get_nf_name(nf, total_flows, zipf_param, churn_fpm, heuristic)
            report_file = DATA_DIR / f"{nf_name}.json"

            if not report_file.is_file():
                print(f"Report file {report_file} does not exist, skipping...")
                found_all = False
                break

            try:
                report = parse_synapse_report(report_file)

                key = Key(
                    s=zipf_param,
                    churn_fpm=churn_fpm,
                )

                values = Values(
                    requested_bps=0,
                    pktgen_bps=0,
                    pktgen_pps=0,
                    dut_ingress_bps=0,
                    dut_ingress_pps=0,
                    dut_egress_bps=report.tput_estimation_bps,
                    dut_egress_pps=report.tput_estimation_pps,
                )

                heatmap_data.add(key, values)
            except Exception as e:
                print(f"Error parsing report {report_file}: {e}")
                found_all = False
                break

        if not found_all:
            print(f"Skipping {nf} with heuristic {heuristic} due to missing reports.")
            continue

        heatmap_output_file = PLOTS_DIR / f"tput_estimated_synapse_{nf}_{heuristic}_heatmap.pdf"
        plot_heatmap_v2(heatmap_data, heatmap_output_file, show_errors=False)


if __name__ == "__main__":
    main()
