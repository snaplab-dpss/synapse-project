#!/usr/bin/env python3

import argparse
import tomli
import os

from pathlib import Path

from experiments.hosts.pktgen import Pktgen
from experiments.hosts.tofino_tg import TofinoTG, TofinoTGController
from experiments.hosts.switch import Switch
from experiments.hosts.synapse import SynapseController
from experiments.throughput import Throughput
from experiments.experiment import Experiment, ExperimentTracker

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
DATA_DIR = CURRENT_DIR / "data"

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default="experiment_config.toml", help="Path to config file")
    parser.add_argument("-f", "--force", action="store_true", default=False, help="Regenerate data, even if they already exist")
    
    args = parser.parse_args()
    
    with open(args.config_file, "rb") as f:
        config = tomli.load(f)
    
    dut_switch = Switch(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    )

    synapse_controller = SynapseController(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        ports=config["devices"]["switch_dut"]["ports"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["controller_dut"],
    )

    tg_switch = TofinoTG(
        hostname=config["hosts"]["switch_tg"],
        repo=config["repo"]["switch_tg"],
        sde=config["devices"]["switch_tg"]["sde"],
        tofino_version=config["devices"]["switch_tg"]["tofino_version"],
        log_file=config["logs"]["switch_tg"],
    )

    tg_controller = TofinoTGController(
        hostname=config["hosts"]["switch_tg"],
        repo=config["repo"]["switch_tg"],
        sde=config["devices"]["switch_tg"]["sde"],
        log_file=config["logs"]["controller_tg"],
    )

    pktgen = Pktgen(
        hostname=config["hosts"]["pktgen"],
        repo=config["repo"]["pktgen"],
        rx_pcie_dev=config["devices"]["pktgen"]["rx_dev"],
        tx_pcie_dev=config["devices"]["pktgen"]["tx_dev"],
        nb_tx_cores=config["devices"]["pktgen"]["nb_tx_cores"],
        log_file=config["logs"]["pktgen"],
    )

    throughput = Throughput(
        name="Forwarder tput",
        save_name=DATA_DIR / "forwarder-tput.csv",
        dut_switch=dut_switch,
        controller=synapse_controller,
        tg_switch=tg_switch,
        tg_controller=tg_controller,
        pktgen=pktgen,
        p4_src_in_repo="tofino/forwarder/forwarder.p4",
        controller_src_in_repo="tofino/forwarder/forwarder.cpp",
        timeout_ms=1000,
        broadcast=[3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32],
        symmetric=[],
        route=[],
        nb_flows=10_000,
        pkt_size=200,
        churn=0,
        experiment_log_file=config["logs"]["experiment"],
    )

    exp_tracker = ExperimentTracker()
    exp_tracker.add_experiments([throughput])
    exp_tracker.run_experiments()

    tg_switch.kill_switchd()

if __name__ == "__main__":
    main()
