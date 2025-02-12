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

from experiments.xput_per_cpu_ratio import ThroughputPerCPURatio

from experiments.experiment import Experiment, ExperimentTracker

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

DATA_DIR = CURRENT_DIR / "data"
APPS_DIR_REPO_RELATIVE = "eval/apps"

PERIODIC_CPU_SENDER_APP = "periodic_cpu_sender"

PACKET_SIZE_BYTES = 64
EXPIRATION_TIME_SEC = 1

TOTAL_FLOWS = 65536

ACTIVE_WAIT_ITERATIONS = [
    0, 1e2, 1e3, 1e4, 1e5, 1e6,
]

console = Console()

def get_periodic_cpu_sender_experiment(
    data_dir: Path,
    switch: Switch,
    controller: Controller,
    pktgen: Pktgen,
    experiment_log_file: str,
) -> list[Experiment]:
    app = PERIODIC_CPU_SENDER_APP

    experiments = []
    for active_wait_iterations in ACTIVE_WAIT_ITERATIONS:
        exp_name = f"xput_cpu_counters_{app}_{int(active_wait_iterations)}_loops"

        experiment = ThroughputPerCPURatio(
            name=exp_name,
            save_name=data_dir / f"{exp_name}.csv",
            switch=switch,
            controller=controller,
            pktgen=pktgen,
            p4_src_in_repo=f"{APPS_DIR_REPO_RELATIVE}/{app}/{app}.p4",
            controller_src_in_repo=f"{APPS_DIR_REPO_RELATIVE}/{app}/{app}.cpp",
            active_wait_iterations=active_wait_iterations,
            nb_flows=TOTAL_FLOWS,
            pkt_size=PACKET_SIZE_BYTES,
            crc_unique_flows=False,
            crc_bits=0,
            experiment_log_file=experiment_log_file,
            console=console,
        )

        experiments.append(experiment)
    
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
    experiments += get_periodic_cpu_sender_experiment(args.out, switch, controller, pktgen, config["logs"]["experiment"])
    
    exp_tracker = ExperimentTracker()
    exp_tracker.add_experiments(experiments)
    exp_tracker.run_experiments()

if __name__ == "__main__":
    main()
