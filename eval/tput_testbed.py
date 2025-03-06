#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from experiments.tput import ThroughputHosts
from experiments.tput_per_pkt_sz import ThroughputPerPacketSize
from experiments.experiment import ExperimentTracker
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

PKT_SIZES = [64, 128, 256, 512, 1024, 1280, 1500]


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)
    
    kill_hosts_on_sigint(config)

    log_file = config["logs"]["experiment"]

    exp_tracker = ExperimentTracker()

    tg_hosts = ThroughputHosts(config, use_accelerator=False)
    exp_tracker.add_experiment(
        ThroughputPerPacketSize(
            name="Echo tput/pkt_sz (TG)",
            save_name=DATA_DIR / "tput_testbed_tg.csv",
            pkt_sizes=PKT_SIZES,
            hosts=tg_hosts,
            p4_src_in_repo="synthesized/synapse-echo.p4",
            controller_src_in_repo="synthesized/synapse-echo.cpp",
            controller_timeout_ms=1000,
            broadcast=config["devices"]["switch_dut"]["client_ports"],
            symmetric=[],
            route=[],
            nb_flows=10_000,
            experiment_log_file=log_file,
        )
    )

    ta_hosts = ThroughputHosts(config, use_accelerator=True)
    exp_tracker.add_experiment(
        ThroughputPerPacketSize(
            name="Echo tput/pkt_sz (TA)",
            save_name=DATA_DIR / "tput_testbed_ta.csv",
            pkt_sizes=PKT_SIZES,
            hosts=ta_hosts,
            p4_src_in_repo="synthesized/synapse-echo.p4",
            controller_src_in_repo="synthesized/synapse-echo.cpp",
            controller_timeout_ms=1000,
            broadcast=config["devices"]["switch_dut"]["client_ports"],
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
