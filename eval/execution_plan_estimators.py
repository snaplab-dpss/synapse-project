#!/usr/bin/env python3

import sys

if sys.version_info < (3, 9, 0):
    raise RuntimeError("Python 3.9 or a more recent version is required.")

import argparse
import tomli
import os

from pathlib import Path

from rich.console import Console

from experiments.hosts.pktgen import Pktgen
from experiments.hosts.switch import Switch
from experiments.hosts.controller import Controller

from experiments.xput_estimator import Ratio, ThroughputEstimator

from experiments.experiment import Experiment, ExperimentTracker

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

DATA_DIR = CURRENT_DIR / "data"
APPS_DIR_REPO_RELATIVE = "eval/apps/exec_plan_estimators"

EP0 = "ep0"
EP1 = "ep1"
EP2 = "ep2"

PACKET_SIZE_BYTES = 64
TOTAL_FLOWS = 65536

RATIOS_MANY = [
    0,
    0.000001,
    0.00001,
    0.0001,
    0.001, 0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008, 0.009,
    0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09,
    0.1, 0.2, 0.3, 0.4, 0.5, 0.6,
    1
]

RATIOS_FEW = [ 0.1, 0.5 ]

console = Console()

def get_ep_estimator_experiments(
    data_dir: Path,
    switch: Switch,
    controller: Controller,
    pktgen: Pktgen,
    experiment_log_file: str,
) -> list[Experiment]:
    configs = [
        (EP0, [
            Ratio(("r0", "--r0", RATIOS_MANY)),
        ]),
        (EP1, [
            Ratio(("r0", "--r0", RATIOS_FEW)),
            Ratio(("r1", "--r1", RATIOS_MANY)),
        ]),
        (EP2, [
            Ratio(("r0", "--r0", RATIOS_FEW)),
            Ratio(("r1", "--r1", RATIOS_MANY)),
            Ratio(("r2", "--r2", RATIOS_FEW)),
        ]),
    ]

    experiments = []

    for app, ratios in configs:
        exp_name = f"xput_ep_estimators_{app}"

        experiment = ThroughputEstimator(
            name=exp_name,
            save_name=data_dir / f"{exp_name}.csv",
            switch=switch,
            controller=controller,
            pktgen=pktgen,
            p4_src_in_repo=f"{APPS_DIR_REPO_RELATIVE}/{app}/{app}.p4",
            controller_src_in_repo=f"{APPS_DIR_REPO_RELATIVE}/{app}/{app}.cpp",
            ratios=ratios,
            nb_flows=TOTAL_FLOWS,
            pkt_size=PACKET_SIZE_BYTES,
            experiment_log_file=experiment_log_file,
            console=console,
            controller_extra_args=[
                ("--iterations", 0),
            ],
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

    experiments = get_ep_estimator_experiments(args.out, switch, controller, pktgen, config["logs"]["experiment"])
    
    exp_tracker = ExperimentTracker()
    exp_tracker.add_experiments(experiments)
    exp_tracker.run_experiments()

if __name__ == "__main__":
    main()
