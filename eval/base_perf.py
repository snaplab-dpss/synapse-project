#!/usr/bin/env python3

import argparse
import tomli
import os

from pathlib import Path

from rich.console import Console

from experiments.hosts.pktgen import Pktgen
from experiments.hosts.switch import Switch
from experiments.hosts.controller import Controller

from experiments.throughput_per_pkt_sz import ThroughputPerPacketSize

from experiments.experiment import Experiment, ExperimentTracker

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

DATA_DIR = CURRENT_DIR / "data"
MICRO_DIR_REPO_RELATIVE = "eval/micro"

# We should find these apps inside MICRO_DIR
TARGET_APPS = [
    "forwarder",
]

console = Console()

def get_throughput_per_pkt_size_experiments(
    data_dir: Path,
    switch: Switch,
    controller: Controller,
    pktgen: Pktgen,
) -> list[Experiment]:
    experiments = []

    for app in TARGET_APPS:
        exp = ThroughputPerPacketSize(
            name=app,
            save_name=data_dir / f"micro-thpt-per-pkt-sz-{app}.csv",
            switch=switch,
            controller=controller,
            pktgen=pktgen,
            p4_src_in_repo=f"{MICRO_DIR_REPO_RELATIVE}/{app}/{app}.p4",
            controller_src_in_repo=f"{MICRO_DIR_REPO_RELATIVE}/{app}/{app}.cpp",
            timeout_ms=0,
            nb_flows=10_000,
            churn=0,
            crc_unique_flows=False,
            crc_bits=0,
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
        sde_install=config["devices"]["switch"]["sde_install"],
        tofino_version=config["devices"]["switch"]["tofino_version"],
        log_file=config["logs"]["switch"],
    )
    
    controller = Controller(
        hostname=config["hosts"]["switch"],
        repo=config["repo"]["switch"],
        sde_install=config["devices"]["switch"]["sde_install"],
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

    experiments = get_throughput_per_pkt_size_experiments(args.out, switch, controller, pktgen)

    exp_tracker = ExperimentTracker()
    exp_tracker.add_experiments(experiments)
    exp_tracker.run_experiments()

if __name__ == "__main__":
    main()
