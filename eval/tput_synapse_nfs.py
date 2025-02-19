#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from experiments.tput import ThroughputHosts, Throughput
from experiments.experiment import Experiment, ExperimentTracker
from utils.constants import *

SYNAPSE_NFS = [
    {
        "name": "echo",
        "description": "Synapse echo",
        "tofino": "synthesized/synapse-echo.p4",
        "controller": "synthesized/synapse-echo.cpp",
        "routing": {
            "broadcast": DUT_CONNECTED_PORTS,
            "symmetric": [],
            "route": [],
        },
    },
    {
        "name": "fwd",
        "description": "Synapse NOP",
        "tofino": "synthesized/synapse-fwd.p4",
        "controller": "synthesized/synapse-fwd.cpp",
        "routing": {
            "broadcast": [ 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31 ],
            "symmetric": [ 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32 ],
            "route": [],
        },
    },
]

def synapse_nfs(hosts: ThroughputHosts, log_file: str) -> list[Experiment]:
    experiments = []

    for nf in SYNAPSE_NFS:
        name = nf["name"]
        description = nf["description"]
        tofino = nf["tofino"]
        controller = nf["controller"]
        routing = nf["routing"]

        experiment = Throughput(
            name=description,
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

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default="experiment_config.toml", help="Path to config file")
    
    args = parser.parse_args()
    
    with open(args.config_file, "rb") as f:
        config = tomli.load(f)
    
    hosts = ThroughputHosts(config)
    log_file = config["logs"]["experiment"]

    exp_tracker = ExperimentTracker()
    exp_tracker.add_experiments(synapse_nfs(hosts, log_file))
    exp_tracker.run_experiments()

    hosts.terminate()

if __name__ == "__main__":
    main()
