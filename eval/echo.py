#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from experiments.tput import ThroughputHosts, Throughput
from experiments.experiment import Experiment, ExperimentTracker
from utils.constants import *


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    hosts = ThroughputHosts(config, use_accelerator=False)

    print("Installing TG")
    hosts.tg_switch.install()

    print("Installing DUT")
    hosts.dut_switch.install("synthesized/synapse-echo.p4")

    print("Launching TG")
    hosts.tg_switch.launch()

    print("Launching DUT controller")
    hosts.dut_controller.launch("synthesized/synapse-echo.cpp")

    print("Launching pktgen")
    hosts.pktgen.launch(pkt_size=1024)

    print("Waiting for TG")
    hosts.tg_switch.wait_ready()

    print("Configuring TG")
    hosts.tg_controller.setup(
        broadcast=config["devices"]["switch_tg"]["dut_ports"],
        symmetric=[],
        route=[],
    )

    print("Waiting for pktgen")
    hosts.pktgen.wait_launch()

    print("Waiting for DUT controller")
    hosts.dut_controller.wait_ready()

    print("Finding stable throughput")
    experiment = Experiment("Echo NF")
    report = experiment.find_stable_throughput(
        tg_controller=hosts.tg_controller,
        pktgen=hosts.pktgen,
        churn=0,
    )

    print(report)

    hosts.terminate()


if __name__ == "__main__":
    main()
