#!/usr/bin/env python3

import argparse
import tomli
import os

from pathlib import Path

from experiments.throughput import ThroughputHosts, Throughput
from experiments.experiment import Experiment, ExperimentTracker

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
DATA_DIR = CURRENT_DIR / "data"

SYNAPSE_NFS = [
    {
        "name": "Synapse NOP",
        "tofino": "synthesized/synapse-nop.p4",
        "controller": "synthesized/synapse-nop.cpp",
        "routing": {
            "broadcast": [ 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31 ],
            "symmetric": [ 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32 ],
            "route": [],
        },
    },
]

def synapse_nfs(hosts: ThroughputHosts, log_file: str) -> list[Experiment]:
    experiments = []

    for nf in SYNAPSE_NFS:
        name = nf["name"]
        tofino = nf["tofino"]
        controller = nf["controller"]
        routing = nf["routing"]

        experiment = Throughput(
            name=name,
            save_name=DATA_DIR / f"tput_{name}.csv",
            hosts=hosts,
            p4_src_in_repo=tofino,
            controller_src_in_repo=controller,
            controller_timeout_ms=1000,
            broadcast=routing["broadcast"],
            symmetric=routing["symmetric"],
            route=routing["route"],
            nb_flows=10_000,
            pkt_size=200,
            churn=0,
            experiment_log_file=log_file,
        )

        experiments.append(experiment)

    return experiments

def simple_experiments(hosts: ThroughputHosts, log_file: str) -> list[Experiment]:
    experiments = []

    forwarder_tput = Throughput(
        name="Forwarder tput",
        save_name=DATA_DIR / "forwarder_tput.csv",
        hosts=hosts,
        p4_src_in_repo="tofino/forwarder/forwarder.p4",
        controller_src_in_repo="tofino/forwarder/forwarder.cpp",
        controller_timeout_ms=1000,
        broadcast=[3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32],
        symmetric=[],
        route=[],
        nb_flows=10_000,
        pkt_size=200,
        churn=0,
        experiment_log_file=log_file,
    )

    send_to_controller_tput = Throughput(
        name="Send to controller tput",
        save_name=DATA_DIR / "send_to_controller_tput.csv",
        hosts=hosts,
        p4_src_in_repo="tofino/send_to_controller/send_to_controller.p4",
        controller_src_in_repo="tofino/send_to_controller/send_to_controller.cpp",
        controller_timeout_ms=1000,
        broadcast=[3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32],
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

    parser.add_argument("-c", "--config-file", type=Path, default="experiment_config.toml", help="Path to config file")
    parser.add_argument("-f", "--force", action="store_true", default=False, help="Regenerate data, even if they already exist")
    
    args = parser.parse_args()
    
    with open(args.config_file, "rb") as f:
        config = tomli.load(f)
    
    hosts = ThroughputHosts(config)
    log_file = config["logs"]["experiment"]

    exp_tracker = ExperimentTracker()
    # exp_tracker.add_experiments(simple_experiments(hosts, log_file))
    exp_tracker.add_experiments(synapse_nfs(hosts, log_file))
    exp_tracker.run_experiments()

    hosts.terminate()

if __name__ == "__main__":
    main()
