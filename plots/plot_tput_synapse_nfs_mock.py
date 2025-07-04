#!/usr/bin/env python3

from itertools import product
import os

from pathlib import Path

from utils.heatmap import *
from utils.synapse_report import *
from utils.parser import parse_heatmap_data_file


CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = CURRENT_DIR.parent

PLOTS_DIR = CURRENT_DIR / "plots"
DATA_DIR = CURRENT_DIR / ".." / "eval" / "data"

MAPTABLE_DATA_FILE = DATA_DIR / "tput_synapse_kvs_maptable.csv"
HHTABLE_DATA_FILE = DATA_DIR / "tput_synapse_kvs_hhtable.csv"
HEATMAP_OUTPUT_FILE = PLOTS_DIR / "tput_synapse_kvs_mock.pdf"


SYNTHESIZED_DATA_DIR = PROJECT_DIR / "synthesized"

TOTAL_FLOWS = [25_000]
CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
HEURISTICS = ["ds-pref-simple", "ds-pref-hhtable"]


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


def get_choices():
    combinations = product("kvs", HEURISTICS)

    choices = {}

    for nf, heuristic in combinations:
        traffic_combinations = product(TOTAL_FLOWS, CHURN_FPM, ZIPF_PARAMS)

        found_all = True
        for total_flows, churn_fpm, zipf_param in traffic_combinations:
            nf_name = get_nf_name("kvs", total_flows, zipf_param, churn_fpm, heuristic)
            report_file = SYNTHESIZED_DATA_DIR / f"{nf_name}.json"

            if not report_file.is_file():
                print(f"Report file {report_file} does not exist, skipping...")
                found_all = False
                break

            try:
                report = parse_synapse_report(report_file)

                key = (churn_fpm, zipf_param)
                if key not in choices:
                    choices[(churn_fpm, zipf_param)] = {"simple": 0, "hhtable": 0}
                if heuristic == "ds-pref-simple":
                    choices[key]["simple"] = report.tput_estimation_bps
                elif heuristic == "ds-pref-hhtable":
                    choices[key]["hhtable"] = report.tput_estimation_bps
            except Exception as e:
                print(f"Error parsing report {report_file}: {e}")
                found_all = False
                break

        if not found_all:
            print(f"Skipping {nf} with heuristic {heuristic} due to missing reports.")
            continue

    return choices


def main():
    maptable_heatmap_data = parse_heatmap_data_file(MAPTABLE_DATA_FILE)
    hhtable_heatmap_data = parse_heatmap_data_file(HHTABLE_DATA_FILE)

    choices = get_choices()

    heatmap_data = HeatmapData()
    for (churn_fpm, zipf_param), values in choices.items():
        print(f"Churn: {churn_fpm}, Zipf: {zipf_param}, Simple: {values['simple']}, HHTable: {values['hhtable']}")

        key = Key(
            s=zipf_param,
            churn_fpm=churn_fpm,
        )

        egress_pps = None

        if values["simple"] > values["hhtable"]:
            egress_pps = maptable_heatmap_data.get(key)
        else:
            egress_pps = hhtable_heatmap_data.get(key)

        for v in egress_pps:
            heatmap_data.add(key, v)

    plot_heatmap(heatmap_data, HEATMAP_OUTPUT_FILE)


if __name__ == "__main__":
    main()
