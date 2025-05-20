#!/usr/bin/env python3

import argparse
import tomli
import itertools

from dataclasses import dataclass
from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional, Callable

from experiments.tput import ThroughputHosts
from experiments.experiment import Experiment, ExperimentTracker
from hosts.kvs_server import KVSServer
from hosts.pktgen import TrafficDist
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

ITERATIONS = 1
PACKET_SIZE = 256


@dataclass
class SynapseNF:
    name: str
    description: str
    data_out: Path
    kvs_mode: bool
    tofino: Path
    controller: Path
    broadcast: Callable[[list[int]], list[int]]
    symmetric: Callable[[list[int]], list[int]]
    route: Callable[[list[int]], list[tuple[int, int]]]
    nb_flows: int


class SynapseThroughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        # Hosts
        tput_hosts: ThroughputHosts,
        # TG controller
        broadcast: list[int],
        symmetric: list[int],
        route: list[tuple[int, int]],
        # Synapse
        p4_src_in_repo: Path,
        controller_src_in_repo: Path,
        dut_ports: list[int],
        # Pktgen
        total_flows: int,
        # Logs
        experiment_log_file: Optional[str] = None,
        console: Console = Console(),
    ) -> None:
        super().__init__(name, experiment_log_file, ITERATIONS)

        # Hosts
        self.tput_hosts = tput_hosts

        # TG controller
        self.broadcast = broadcast
        self.symmetric = symmetric
        self.route = route

        # Synapse
        self.p4_src_in_repo = p4_src_in_repo
        self.controller_src_in_repo = controller_src_in_repo
        self.dut_ports = dut_ports

        # Pktgen
        self.total_flows = total_flows

        self.console = console

        self._sync()

    def _sync(self):
        pass

    def run(
        self,
        step_progress: Progress,
        current_iter: int,
    ) -> None:
        self.log("Installing Tofino TG")
        self.tput_hosts.tg_switch.install()

        self.log("Launching Tofino TG")
        self.tput_hosts.tg_switch.launch()

        self.log("Installing Synapse P4 program")
        self.tput_hosts.dut_switch.install(
            src_in_repo=self.p4_src_in_repo,
        )

        self.log("Launching Synapse controller")
        self.tput_hosts.dut_controller.launch(
            src_in_repo=self.controller_src_in_repo,
            ports=self.dut_ports,
        )

        self.log("Launching pktgen")
        self.tput_hosts.pktgen.launch(kvs_mode=False)

        self.log("Waiting for Tofino TG")
        self.tput_hosts.tg_switch.wait_ready()

        self.log("Configuring Tofino TG")
        self.tput_hosts.tg_controller.setup(
            broadcast=self.broadcast,
            symmetric=self.symmetric,
            route=self.route,
        )

        self.log("Waiting for pktgen")
        self.tput_hosts.pktgen.wait_launch()

        self.log("Starting experiment")

        self.log("Waiting for the Synapse controller")
        self.tput_hosts.dut_controller.wait_ready()

        self.log("Launching pktgen")
        self.tput_hosts.pktgen.close()
        self.tput_hosts.pktgen.launch(
            nb_flows=self.total_flows,
            traffic_dist=TrafficDist.UNIFORM,
            kvs_mode=False,
            pkt_size=PACKET_SIZE,
        )

        self.tput_hosts.pktgen.wait_launch()

        report = self.find_stable_throughput(
            tg_controller=self.tput_hosts.tg_controller,
            pktgen=self.tput_hosts.pktgen,
            churn=0,
        )

        print(report)

        self.tput_hosts.pktgen.close()
        self.tput_hosts.dut_controller.quit()


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    kill_hosts_on_sigint(config)

    log_file = config["logs"]["experiment"]

    tput_hosts = ThroughputHosts(
        config,
        use_accelerator=False,
    )

    kvs_server = KVSServer(
        hostname=config["hosts"]["server"],
        repo=config["repo"]["server"],
        pcie_dev=config["devices"]["server"]["dev"],
        log_file=config["logs"]["server"],
    )

    tg_dut_ports = config["devices"]["switch_tg"]["dut_ports"]
    symmetric = []
    route = []

    exp_tracker = ExperimentTracker()

    echo_nf = SynapseNF(
        name="echo",
        description="Synapse echo",
        data_out=Path("tput_synapse_echo.csv"),
        kvs_mode=False,
        tofino=Path("synthesized/synapse-echo.p4"),
        controller=Path("synthesized/synapse-echo.cpp"),
        broadcast=lambda ports: ports,
        symmetric=lambda _: [],
        route=lambda _: [],
        nb_flows=40_000,
    )

    broadcast = echo_nf.broadcast(tg_dut_ports)
    symmetric = echo_nf.symmetric(tg_dut_ports)
    route = echo_nf.route(tg_dut_ports)

    # Force a copy of the list to avoid modifying the original list.
    dut_ports = list(config["devices"]["switch_dut"]["client_ports"])

    exp_tracker.add_experiment(
        SynapseThroughput(
            name=echo_nf.description,
            tput_hosts=tput_hosts,
            broadcast=broadcast,
            symmetric=symmetric,
            route=route,
            p4_src_in_repo=echo_nf.tofino,
            controller_src_in_repo=echo_nf.controller,
            dut_ports=dut_ports,
            total_flows=echo_nf.nb_flows,
            experiment_log_file=log_file,
        )
    )

    exp_tracker.run_experiments()

    tput_hosts.terminate()
    kvs_server.kill_server()


if __name__ == "__main__":
    main()
