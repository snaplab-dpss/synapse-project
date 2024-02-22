#!/usr/bin/env python3

import sys
import click
import tomli
import os

from pathlib import Path

from rich.console import Console

from util.hosts.remote import RemoteHost

from util.hosts.pktgen import Pktgen
from util.hosts.switch import Switch
from util.hosts.controller import Controller

from util.experiment import Experiment, ExperimentTracker
from util.throughput import Throughput

if sys.version_info < (3, 9, 0):
    raise RuntimeError("Python 3.9 or a more recent version is required.")

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
MICRO_DIR   = CURRENT_DIR / Path("micro")
DATA_DIR    = CURRENT_DIR / Path("data")

console = Console()

@click.command()
@click.option(
    "--config-file",
    "-c",
    type=click.Path(exists=True),
    default="experiment_config.toml",
    show_default=True,
    help="Path to config file.",
)
def main(
    config_file,
):
    with open(config_file, "rb") as f:
        config = tomli.load(f)
    
    print("Launching controller...")
    controller = Controller(
        hostname=config["hosts"]["switch"],
        repo=config["repo"]["switch"],
        tofino_version=config["devices"]["switch"]["tofino_version"],
        log_file=config["logs"]["controller"],
    )

    print("Launching switch...")
    switch = Switch(
        hostname=config["hosts"]["switch"],
        repo=config["repo"]["switch"],
        tofino_version=config["devices"]["switch"]["tofino_version"],
        log_file=config["logs"]["switch"],
    )

    print("Launching pktgen...")
    pktgen = Pktgen(
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
