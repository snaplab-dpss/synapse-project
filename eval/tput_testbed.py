#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from experiments.tput import ThroughputHosts
from experiments.tput_per_pkt_sz import ThroughputPerPacketSize
from experiments.experiment import ExperimentTracker
from experiments.hosts.tofino_ta import TofinoTA, TofinoTAController
from utils.constants import *

PKT_SIZES = [64, 128, 256, 512, 1024, 1280, 1500]


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default="experiment_config.toml", help="Path to config file")
    parser.add_argument("-f", "--force", action="store_true", default=False, help="Regenerate data, even if they already exist")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    tg_hosts = ThroughputHosts(config)
    log_file = config["logs"]["experiment"]

    exp_tracker = ExperimentTracker()

    exp_tracker.add_experiment(
        ThroughputPerPacketSize(
            name="Echo tput/pkt_sz (TG)",
            save_name=DATA_DIR / "tput_testbed_tg.csv",
            pkt_sizes=PKT_SIZES,
            hosts=tg_hosts,
            p4_src_in_repo="synthesized/synapse-echo.p4",
            controller_src_in_repo="synthesized/synapse-echo.cpp",
            controller_timeout_ms=1000,
            broadcast=[3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32],
            symmetric=[],
            route=[],
            nb_flows=10_000,
            experiment_log_file=log_file,
        )
    )

    ta_hosts = ThroughputHosts(config)
    ta_hosts.tg_switch = TofinoTA(
        hostname=config["hosts"]["switch_tg"],
        repo=config["repo"]["switch_tg"],
        sde=config["devices"]["switch_tg"]["sde"],
        tofino_version=config["devices"]["switch_tg"]["tofino_version"],
        log_file=config["logs"]["switch_tg"],
    )

    ta_hosts.tg_controller = TofinoTAController(
        hostname=config["hosts"]["switch_tg"],
        repo=config["repo"]["switch_tg"],
        sde=config["devices"]["switch_tg"]["sde"],
        log_file=config["logs"]["controller_tg"],
    )

    exp_tracker.add_experiment(
        ThroughputPerPacketSize(
            name="Echo tput/pkt_sz (TA)",
            save_name=DATA_DIR / "tput_testbed_ta.csv",
            pkt_sizes=PKT_SIZES,
            hosts=ta_hosts,
            p4_src_in_repo="synthesized/synapse-echo.p4",
            controller_src_in_repo="synthesized/synapse-echo.cpp",
            controller_timeout_ms=1000,
            broadcast=[3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32],
            symmetric=[],
            route=[],
            nb_flows=10_000,
            experiment_log_file=log_file,
        )
    )

    exp_tracker.run_experiments()

    tg_hosts.terminate()
    ta_hosts.terminate()


if __name__ == "__main__":
    main()
