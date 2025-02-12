#!/usr/bin/env python3

import sys

if sys.version_info < (3, 9, 0):
    raise RuntimeError("Python 3.9 or a more recent version is required.")

import math
import argparse
import tomli
import os

from pathlib import Path

from rich.console import Console

from experiments.hosts.pktgen import Pktgen
from experiments.hosts.switch import Switch
from eval.experiments.hosts.synapse import Controller

from experiments.xput_cpu_counters_under_churn import ThroughputWithCPUCountersUnderChurn

from experiments.experiment import Experiment, ExperimentTracker

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

DATA_DIR = CURRENT_DIR / "data"
APPS_DIR_REPO_RELATIVE = "eval/apps/data_structures"

TARGET_TABLE_PERF_UNDER_CHURN_APPS = [
    "table_map",
]

TARGET_CACHE_PERF_UNDER_CHURN_APPS = [
    "cached_table_map",
    "transient_cached_table_map",
]

CACHE_SIZE = 32768
CACHE_OCCUPANCY = [ 0.25, 0.5, 0.75, 1, 2, 4 ]

MAX_TOTAL_FLOWS = 131072
PACKET_SIZE_BYTES = 64
EXPIRATION_TIME_MILISEC = 100

console = Console()

def get_cache_perf_experiments(
    data_dir: Path,
    switch: Switch,
    controller: Controller,
    pktgen: Pktgen,
    experiment_log_file: str,
) -> list[Experiment]:
    experiments = []

    for app in TARGET_CACHE_PERF_UNDER_CHURN_APPS:
        for cache_occupancy in CACHE_OCCUPANCY:
            total_flows = int(CACHE_SIZE / cache_occupancy)

            # Total number of flows must be even
            if total_flows % 2 != 0:
                total_flows += 1

            cache_size_exp_base_2 = int(math.log(CACHE_SIZE, 2))
            exp_name = f"churn_{app}_{total_flows}_flows_{CACHE_SIZE}_cache"

            assert total_flows <= MAX_TOTAL_FLOWS

            exp = ThroughputWithCPUCountersUnderChurn(
                name=exp_name,
                save_name=data_dir / f"{exp_name}.csv",
                lo_churn_fpm=10_000,
                mid_churn_fpm=50_000,
                hi_churn_fpm=90_000,
                switch=switch,
                controller=controller,
                pktgen=pktgen,
                p4_src_in_repo=f"{APPS_DIR_REPO_RELATIVE}/{app}/{app}.p4",
                controller_src_in_repo=f"{APPS_DIR_REPO_RELATIVE}/{app}/{app}.cpp",
                timeout_ms=EXPIRATION_TIME_MILISEC,
                nb_flows=total_flows,
                pkt_size=PACKET_SIZE_BYTES,
                crc_unique_flows=False,
                crc_bits=0,
                p4_compile_time_vars=[
                    ("CACHE_CAPACITY_EXPONENT_BASE_2", str(cache_size_exp_base_2)),
                    ("EXPIRATION_TIME_DECISEC", str(int(EXPIRATION_TIME_MILISEC / 100))),
                ],
                experiment_log_file=experiment_log_file,
                console=console,
            )

            experiments.append(exp)
    
    return experiments

def get_table_perf_experiments(
    data_dir: Path,
    switch: Switch,
    controller: Controller,
    pktgen: Pktgen,
    experiment_log_file: str,
) -> list[Experiment]:
    experiments = []

    for app in TARGET_TABLE_PERF_UNDER_CHURN_APPS:
        exp_name = f"churn_{app}"
        exp = ThroughputWithCPUCountersUnderChurn(
            name=exp_name,
            save_name=data_dir / f"{exp_name}.csv",
            lo_churn_fpm=5_000,
            mid_churn_fpm=10_000,
            hi_churn_fpm=100_000,
            switch=switch,
            controller=controller,
            pktgen=pktgen,
            p4_src_in_repo=f"{APPS_DIR_REPO_RELATIVE}/{app}/{app}.p4",
            controller_src_in_repo=f"{APPS_DIR_REPO_RELATIVE}/{app}/{app}.cpp",
            timeout_ms=EXPIRATION_TIME_MILISEC,
            nb_flows=65536,
            pkt_size=PACKET_SIZE_BYTES,
            crc_unique_flows=False,
            crc_bits=0,
            experiment_log_file=experiment_log_file,
            console=console,
        )

        experiments.append(exp)
    
    return experiments

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default="experiment_config.toml", help="Path to config file")
    parser.add_argument("--out", type=Path, default=DATA_DIR, help="Output directory")
    parser.add_argument("-f", "--force", action="store_true", default=False,
                        help="Regenerate data/plots, even if they already exist")
    
    args = parser.parse_args()
    
    with open(args.config_file, "rb") as f:
        config = tomli.load(f)
    
    switch = Switch(
        hostname=config["hosts"]["switch"],
        repo=config["repo"]["switch"],
        sde=config["devices"]["switch"]["sde"],
        tofino_version=config["devices"]["switch"]["tofino_version"],
        log_file=config["logs"]["switch"],
    )
    
    controller = Controller(
        hostname=config["hosts"]["switch"],
        repo=config["repo"]["switch"],
        sde=config["devices"]["switch"]["sde"],
        switch_pktgen_in_port=config["devices"]["switch"]["pktgen_inbound_port"],
        switch_pktgen_out_port=config["devices"]["switch"]["pktgen_outbound_port"],
        tofino_version=config["devices"]["switch"]["tofino_version"],
        log_file=config["logs"]["controller"],
    )

    pktgen = Pktgen(
        hostname=config["hosts"]["pktgen"],
        repo=config["repo"]["pktgen"],
        rx_pcie_dev=config["devices"]["pktgen"]["rx_dev"],
        tx_pcie_dev=config["devices"]["pktgen"]["tx_dev"],
        nb_tx_cores=config["devices"]["pktgen"]["nb_tx_cores"],
        log_file=config["logs"]["pktgen"],
    )

    experiments = []
    experiments += get_table_perf_experiments(args.out, switch, controller, pktgen, config["logs"]["experiment"])
    experiments += get_cache_perf_experiments(args.out, switch, controller, pktgen, config["logs"]["experiment"])
    
    exp_tracker = ExperimentTracker()
    exp_tracker.add_experiments(experiments)
    exp_tracker.run_experiments()

if __name__ == "__main__":
    main()
