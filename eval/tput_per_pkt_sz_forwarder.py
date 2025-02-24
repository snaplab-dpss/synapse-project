#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from experiments.tput import ThroughputHosts
from experiments.tput_per_pkt_sz import ThroughputPerPacketSize
from experiments.experiment import ExperimentTracker
from utils.constants import *

PKT_SIZES = [64, 128, 256, 512, 1024, 1280, 1500]


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    hosts = ThroughputHosts(config)
    log_file = config["logs"]["experiment"]

    exp_tracker = ExperimentTracker()

    exp_tracker.add_experiment(
        ThroughputPerPacketSize(
            name="Forwarder tput/pkt_sz",
            save_name=DATA_DIR / "tput_per_pkt_size_forwarder.csv",
            pkt_sizes=PKT_SIZES,
            hosts=hosts,
            p4_src_in_repo="tofino/forwarder/forwarder.p4",
            controller_src_in_repo="tofino/forwarder/forwarder.cpp",
            controller_timeout_ms=1000,
            broadcast=[3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32],
            symmetric=[],
            route=[],
            nb_flows=10_000,
            experiment_log_file=log_file,
        )
    )

    exp_tracker.run_experiments()

    hosts.terminate()


if __name__ == "__main__":
    main()
