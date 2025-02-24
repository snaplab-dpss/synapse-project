#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from hosts.pktgen import Pktgen
from hosts.kvs_server import KVSServer
from hosts.switch import Switch
from hosts.synapse import SynapseController
from utils.constants import *


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    print("Launching Tofino DUT...")
    Switch(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    )

    print("Launching Tofino TG...")
    Switch(
        hostname=config["hosts"]["switch_tg"],
        repo=config["repo"]["switch_tg"],
        sde=config["devices"]["switch_tg"]["sde"],
        tofino_version=config["devices"]["switch_tg"]["tofino_version"],
        log_file=config["logs"]["switch_tg"],
    )

    print("Launching controller for Tofino DUT...")
    SynapseController(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        ports=config["devices"]["switch_dut"]["ports"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["controller_dut"],
    )

    print("Launching pktgen...")
    Pktgen(
        hostname=config["hosts"]["pktgen"],
        repo=config["repo"]["pktgen"],
        rx_pcie_dev=config["devices"]["pktgen"]["rx_dev"],
        tx_pcie_dev=config["devices"]["pktgen"]["tx_dev"],
        nb_tx_cores=config["devices"]["pktgen"]["nb_tx_cores"],
        log_file=config["logs"]["pktgen"],
    )

    print("Launching KVS server...")
    server = KVSServer(
        hostname=config["hosts"]["server"],
        repo=config["repo"]["server"],
        pcie_devs=config["devices"]["server"]["devs"],
        log_file=config["logs"]["server"],
    )

    server.launch()
    server.wait_launch()

    print(f"All hosts are reachable.")


if __name__ == "__main__":
    main()
