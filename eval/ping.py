#!/usr/bin/env python3

import argparse
import tomli
import os

from pathlib import Path

from experiments.hosts.pktgen import Pktgen
from experiments.hosts.switch import Switch
from experiments.hosts.controller import Controller

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default="experiment_config.toml", help="Path to config file")
    
    args = parser.parse_args()
    
    with open(args.config_file, "rb") as f:
        config = tomli.load(f)
    
    print("Launching switch...")
    Switch(
        hostname=config["hosts"]["switch"],
        repo=config["repo"]["switch"],
        sde_install=config["devices"]["switch"]["sde_install"],
        tofino_version=config["devices"]["switch"]["tofino_version"],
        log_file=config["logs"]["switch"],
    )

    print("Launching controller...")
    Controller(
        hostname=config["hosts"]["switch"],
        repo=config["repo"]["switch"],
        sde_install=config["devices"]["switch"]["sde_install"],
        switch_pktgen_in_port=config["devices"]["switch"]["pktgen_inbound_port"],
        switch_pktgen_out_port=config["devices"]["switch"]["pktgen_outbound_port"],
        tofino_version=config["devices"]["switch"]["tofino_version"],
        log_file=config["logs"]["controller"],
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

    print(f"All hosts are reachable.")

if __name__ == "__main__":
    main()
