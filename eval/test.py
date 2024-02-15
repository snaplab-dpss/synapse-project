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
from util.churn import Churn

if sys.version_info < (3, 9, 0):
    raise RuntimeError("Python 3.9 or a more recent version is required.")

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
MICRO_DIR   = CURRENT_DIR / Path("micro")
DATA_DIR    = CURRENT_DIR / Path("data")

console = Console()

def send_sources(
    config: dict,
    name: str,
    local_switch: Path,
    local_controller: Path,
) -> tuple[Path, Path]:
    name = Path(name).stem

    remote_switch     = Path(config["paths"]["switch"]) / f"{name}{local_switch.suffix}"
    remote_controller = Path(config["paths"]["switch"]) / f"{name}{local_controller.suffix}"

    switch = RemoteHost(config["hosts"]["switch"])
    switch.upload_file(local_switch, remote_switch, overwrite=True)

    controller = RemoteHost(config["hosts"]["controller"])
    controller.upload_file(local_controller, remote_controller, overwrite=True)
    
    return remote_switch, remote_controller

def get_test_experiment(config: dict, data_dir: Path) -> Experiment:
    pkt_size = 64
    nb_flows = 1024

    exp_name   = f"redirector throughput"
    data_fname = f"redirector_throughput.csv"

    switch_src     = MICRO_DIR / Path("redirector") / "redirector.p4"
    controller_src = MICRO_DIR / Path("redirector") / "redirector.cpp"

    switch_src, controller_src = send_sources(config, exp_name, switch_src, controller_src)

    pktgen = Pktgen(
        hostname=config["hosts"]["pktgen"],
        rx_pcie_dev=config["devices"]["pktgen"]["rx_dev"],
        tx_pcie_dev=config["devices"]["pktgen"]["tx_dev"],
        nb_tx_cores=config["devices"]["pktgen"]["nb_tx_cores"],
        nb_flows=nb_flows,
        pkt_size=pkt_size,
        crc_unique_flows=False,
        pktgen_exe=Path(config["paths"]["pktgen"]),
        log_file=config["logs"]["pktgen"],
    )

    switch = Switch(
        hostname=config["hosts"]["switch"],
        src=switch_src,
        compiler=config["paths"]["dataplane_build_script"],
        log_file=config["logs"]["controller"],
    )

    controller = Controller(
        hostname=config["hosts"]["controller"],
        src=controller_src,
        builder=config["paths"]["controller_build_script"],
        sde_install=config["paths"]["sde_install"],
        log_file=config["logs"]["controller"],
    )

    throughput = Throughput(
        name=exp_name,
        iterations=1,
        save_name=data_dir / Path(data_fname),
        switch=switch,
        controller=controller,
        pktgen=pktgen,
        churn=0,
        console=console,
    )

    return throughput

@click.command()
@click.option(
    "--data-dir",
    "-d",
    type=click.Path(),
    default=DATA_DIR,
    show_default=True,
    help="Path to output data directory.",
)
@click.option(
    "--config-file",
    "-c",
    type=click.Path(exists=True),
    default="experiment_config.toml",
    show_default=True,
    help="Path to config file.",
)
def main(
    data_dir,
    config_file,
):
    data_dir = Path(data_dir)
    data_dir.mkdir(parents=True, exist_ok=True)

    with open(config_file, "rb") as f:
        config = tomli.load(f)
    
    exp_tracker = ExperimentTracker()

    experiment = get_test_experiment(config, data_dir)
    exp_tracker.add_experiment(experiment)

    exp_tracker.run_experiments()

if __name__ == "__main__":
    main()
