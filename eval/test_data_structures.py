#!/usr/bin/env python3

import argparse
import tomli

from dataclasses import dataclass
from pathlib import Path

from typing import Optional, Callable

from experiments.tput import ThroughputHosts
from experiments.experiment import Experiment
from hosts.kvs_server import KVSServer
from hosts.pktgen import TrafficDist
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

TOTAL_FLOWS = 50_000
CHURN_FPM = 0
ZIPF_PARAM = 1


@dataclass
class NF:
    name: str
    description: str
    data_out: Path
    tofino: Path
    controller: Path
    broadcast: Callable[[list[int]], list[int]]
    symmetric: Callable[[list[int]], list[int]]
    route: Callable[[list[int]], list[tuple[int, int]]]


NFS = [
    NF(
        name="map_table",
        description="MapTable",
        data_out=Path("tput_map_table.csv"),
        tofino=Path("tofino/data_structures/map_table/map_table.p4"),
        controller=Path("tofino/data_structures/map_table/map_table.cpp"),
        broadcast=lambda ports: ports,
        symmetric=lambda _: [],
        route=lambda _: [],
    ),
]


class Test(Experiment):
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
        zipf_param: float,
        churn_fpm: int,
        # Logs
        experiment_log_file: Optional[str] = None,
    ) -> None:
        super().__init__(name, experiment_log_file, 1)

        # Hosts
        self.tput_hosts = tput_hosts

        # TG controller
        self.broadcast = broadcast
        self.symmetric = symmetric
        self.route = route

        # NF
        self.p4_src_in_repo = p4_src_in_repo
        self.controller_src_in_repo = controller_src_in_repo
        self.dut_ports = dut_ports

        # Pktgen
        self.total_flows = total_flows
        self.zipf_param = zipf_param
        self.churn_fpm = churn_fpm

    def run(self) -> None:
        self.log("Installing Tofino TG")
        self.tput_hosts.tg_switch.install()

        self.log("Launching Tofino TG")
        self.tput_hosts.tg_switch.launch()

        self.log("Installing P4 program")
        self.tput_hosts.dut_switch.install(
            src_in_repo=self.p4_src_in_repo,
        )

        self.log("Launching controller")
        self.tput_hosts.dut_controller.launch(
            src_in_repo=self.controller_src_in_repo,
            ports=self.dut_ports,
        )

        self.log("Launching pktgen")
        self.tput_hosts.pktgen.launch()

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

        self.log("Waiting for controller")
        self.tput_hosts.dut_controller.wait_ready()

        self.log("Launching pktgen")
        self.tput_hosts.pktgen.close()
        self.tput_hosts.pktgen.launch(
            nb_flows=self.total_flows,
            traffic_dist=TrafficDist.ZIPF,
            zipf_param=self.zipf_param,
        )

        self.tput_hosts.pktgen.wait_launch()

        report = self.find_stable_throughput(
            tg_controller=self.tput_hosts.tg_controller,
            pktgen=self.tput_hosts.pktgen,
            churn=self.churn_fpm,
        )

        tx_Gbps = report.dut_ingress_bps / 1e9
        tx_Mpps = report.dut_ingress_pps / 1e6

        rx_Gbps = report.dut_egress_bps / 1e9
        rx_Mpps = report.dut_egress_pps / 1e6

        print(f"Experiment: {self.name}")
        print(f"TX     {tx_Mpps:12.5f} Mpps {tx_Gbps:12.5f} Gbps")
        print(f"RX     {rx_Mpps:12.5f} Mpps {rx_Gbps:12.5f} Gbps")

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

    tg_dut_ports = config["devices"]["switch_tg"]["dut_ports"]
    symmetric = []
    route = []

    for nf in NFS:
        broadcast = nf.broadcast(tg_dut_ports)
        symmetric = nf.symmetric(tg_dut_ports)
        route = nf.route(tg_dut_ports)

        # Force a copy of the list to avoid modifying the original list.
        dut_ports = list(config["devices"]["switch_dut"]["client_ports"])

        experiment = Test(
            name=nf.description,
            tput_hosts=tput_hosts,
            broadcast=broadcast,
            symmetric=symmetric,
            route=route,
            p4_src_in_repo=nf.tofino,
            controller_src_in_repo=nf.controller,
            dut_ports=dut_ports,
            total_flows=TOTAL_FLOWS,
            zipf_param=ZIPF_PARAM,
            churn_fpm=CHURN_FPM,
            experiment_log_file=log_file,
        )

        experiment.run()

    tput_hosts.terminate()


if __name__ == "__main__":
    main()
