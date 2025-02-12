#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from experiments.hosts.pktgen import Pktgen
from experiments.hosts.tofino_tg import TofinoTG, TofinoTGController
from experiments.hosts.synapse import SynapseController

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default="experiment_config.toml", help="Path to config file")
    
    args = parser.parse_args()
    
    with open(args.config_file, "rb") as f:
        config = tomli.load(f)
    
    # print("Launching Tofino DUT...")
    # synapse_nf = Switch(
    #     hostname=config["hosts"]["switch_dut"],
    #     repo=config["repo"]["switch_dut"],
    #     sde=config["devices"]["switch_dut"]["sde"],
    #     tofino_version=config["devices"]["switch_dut"]["tofino_version"],
    #     log_file=config["logs"]["switch_dut"],
    # )

    # print("Launching controller for Tofino DUT...")
    # synapse_controller = SynapseController(
    #     hostname=config["hosts"]["switch_dut"],
    #     repo=config["repo"]["switch_dut"],
    #     sde=config["devices"]["switch_dut"]["sde"],
    #     ports=config["devices"]["switch_dut"]["ports"],
    #     tofino_version=config["devices"]["switch_dut"]["tofino_version"],
    #     log_file=config["logs"]["controller_dut"],
    # )

    print("Launching Tofino TG...")
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

    # print("Launching pktgen...")
    # Pktgen(
    #     hostname=config["hosts"]["pktgen"],
    #     repo=config["repo"]["pktgen"],
    #     rx_pcie_dev=config["devices"]["pktgen"]["rx_dev"],
    #     tx_pcie_dev=config["devices"]["pktgen"]["tx_dev"],
    #     nb_tx_cores=config["devices"]["pktgen"]["nb_tx_cores"],
    #     log_file=config["logs"]["pktgen"],
    # )

    tg_switch.install()
    tg_switch.launch()
    tg_switch.wait_ready()
    tg_controller.run(
        broadcast=[3,4,5,6],
        symmetric=[],
        route=[],
    )

if __name__ == "__main__":
    main()
