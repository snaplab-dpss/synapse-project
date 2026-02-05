#!/usr/bin/env python3

import os

from pathlib import Path
from itertools import product
from statistics import mean, stdev
from prettytable import PrettyTable

from tofino_resource_parser import calculate_lines_of_p4_code, parse_tofino_resources_file, Resources
from tput_eval_data import parse_tput_eval_data_file, EvalDataKey


CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / ".." / "..").resolve()
DATA_DIR = PROJECT_DIR / "eval" / "data"

STRAW_TPUT_DATA = DATA_DIR / "tput_gallium_kvs.csv"
NETCACHE_TPUT_DATA = DATA_DIR / "tput_netcache.csv"
SWITCHAROO_TPUT_DATA = DATA_DIR / "tput_switcharoo.csv"

NAIVE_RESOURCES_FILE = PROJECT_DIR / "tofino" / "nfs" / "kvstore" / "kvs-naive-resources.txt"
STRAW_RESOURCES_FILE = PROJECT_DIR / "tofino" / "nfs" / "kvstore" / "kvs-straw-resources.txt"
NETCACHE_RESOURCES_FILE = PROJECT_DIR / "tofino" / "netcache" / "p4" / "netcache-resources.txt"
SWITCHAROO_RESOURCES_FILE = PROJECT_DIR / "tofino" / "switcharoo" / "p4" / "switcharoo-resources.txt"

TARGET_NF = "kvs"
TOTAL_FLOWS = 40_000

DESIGNS = {
    "Naive": (None, NAIVE_RESOURCES_FILE),
    "Straw": (STRAW_TPUT_DATA, STRAW_RESOURCES_FILE),
    "NetCache": (NETCACHE_TPUT_DATA, NETCACHE_RESOURCES_FILE),
    "Switcharoo": (SWITCHAROO_TPUT_DATA, SWITCHAROO_RESOURCES_FILE),
}

chosen_workloads = [
    EvalDataKey(skew=0.4, churn=0),
    EvalDataKey(skew=1.2, churn=0),
    EvalDataKey(skew=1.2, churn=1_000_000),
]


def whole_number_to_label(n: int) -> str:
    if n < 1e3:
        return f"{n}"
    elif n < 1e6:
        return f"{int(round(n / 1e3, 2))}K"
    elif n < 1e9:
        return f"{int(round(n / 1e6, 2))}M"
    elif n < 1e12:
        return f"{int(round(n / 1e9, 2))}G"
    else:
        return f"{int(round(n / 1e12, 2))}T"


if __name__ == "__main__":
    data_per_design: dict[str, tuple[Resources, dict[EvalDataKey, int]]] = {}

    for design, (tput_file, resources_file) in DESIGNS.items():
        assert resources_file.exists(), f"Synthesized resources file {resources_file} does not exist!"
        resources = parse_tofino_resources_file(resources_file)
        if design not in data_per_design:
            data_per_design[design] = (resources, {workload: 0 for workload in chosen_workloads})

        for workload in chosen_workloads:
            pps = int(parse_tput_eval_data_file(tput_file)[workload].get_avg_pps()[0]) if tput_file is not None else 0
            data_per_design[design][1][workload] = pps

    table = PrettyTable()
    table.field_names = [
        "Design",
        "Stages (%)",
        "SRAM (%)",
    ] + [f"s={workload.skew} | {whole_number_to_label(workload.churn)}fpm" for workload in chosen_workloads]
    for design, (resources, entries) in data_per_design.items():
        columns = [
            design,
            f"{100*resources.stages:4.1f}",
            f"{100*resources.sram:4.0f}",
        ]
        columns += [f"{entries[workload]/1e6:.0f}" for workload in chosen_workloads]
        table.add_row(columns)

    print(table)
