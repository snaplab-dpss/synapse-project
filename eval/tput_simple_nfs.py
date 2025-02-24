#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from experiments.tput import ThroughputHosts, Throughput
from experiments.experiment import Experiment, ExperimentTracker
from utils.constants import *


def simple_experiments(hosts: ThroughputHosts, log_file: str) -> list[Experiment]:
    experiments = []

    forwarder_tput = Throughput(
        name="Forwarder tput",
        save_name=DATA_DIR / "tput_forwarder.csv",
        hosts=hosts,
        p4_src_in_repo="tofino/forwarder/forwarder.p4",
        controller_src_in_repo="tofino/forwarder/forwarder.cpp",
        controller_timeout_ms=1000,
        broadcast=[3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32],
        symmetric=[],
        route=[],
        nb_flows=10_000,
        pkt_size=200,
        churn=0,
        experiment_log_file=log_file,
    )

    send_to_controller_tput = Throughput(
        name="Send to controller tput",
        save_name=DATA_DIR / "tput_send_to_controller.csv",
        hosts=hosts,
        p4_src_in_repo="tofino/send_to_controller/send_to_controller.p4",
        controller_src_in_repo="tofino/send_to_controller/send_to_controller.cpp",
        controller_timeout_ms=1000,
        broadcast=[3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32],
        symmetric=[],
        route=[],
        nb_flows=10_000,
        pkt_size=200,
        churn=0,
        experiment_log_file=log_file,
    )

    experiments.append(forwarder_tput)
    experiments.append(send_to_controller_tput)

    return experiments


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    hosts = ThroughputHosts(config)
    log_file = config["logs"]["experiment"]

    exp_tracker = ExperimentTracker()
    exp_tracker.add_experiments(simple_experiments(hosts, log_file))
    exp_tracker.run_experiments()

    hosts.terminate()


if __name__ == "__main__":
    main()
