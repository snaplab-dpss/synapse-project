#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from typing import Optional

from experiments.tput import TGHosts
from experiments.experiment import Experiment
from hosts.kvs_server import KVSServer
from hosts.switcharoo import Switcharoo, SwitcharooController
from hosts.pktgen import TrafficDist
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

STORAGE_SERVER_DELAY_NS = 0
TOTAL_FLOWS = 25_000
KVS_GET_RATIO = 0.99
CHURN_FPM = 1_000_000
ZIPF_PARAM = 0

EXPERIMENT_NAME = "Switcharoo throughput"


class SwitcharooThroughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        delay_ns: int,
        # Hosts
        tg_hosts: TGHosts,
        switcharoo: Switcharoo,
        switcharoo_controller: SwitcharooController,
        kvs_server: KVSServer,
        # TG controller
        broadcast: list[int],
        symmetric: list[int],
        route: list[tuple[int, int]],
        # Pktgen
        total_flows: int,
        zipf_param: float,
        churn_fpm: int,
        # Logs
        experiment_log_file: Optional[str] = None,
    ) -> None:
        super().__init__(name, experiment_log_file, 1)

        # Experiment parameters
        self.delay_ns = delay_ns

        # Hosts
        self.tg_hosts = tg_hosts
        self.switcharoo = switcharoo
        self.switcharoo_controller = switcharoo_controller
        self.kvs_server = kvs_server

        # TG controller
        self.broadcast = broadcast
        self.symmetric = symmetric
        self.route = route

        # Pktgen
        self.total_flows = total_flows
        self.zipf_param = zipf_param
        self.churn_fpm = churn_fpm

    def run(self) -> None:
        self.log("Installing Tofino TG")
        self.tg_hosts.tg_switch.install()

        self.log("Launching Tofino TG")
        self.tg_hosts.tg_switch.launch()

        self.log("Installing Switcharoo")
        self.switcharoo.install()

        self.log("Launching Switcharoo")
        self.switcharoo.launch()

        self.log("Launching pktgen")
        self.tg_hosts.pktgen.launch(kvs_mode=True)

        self.log("Waiting for Tofino TG")
        self.tg_hosts.tg_switch.wait_ready()

        self.log("Waiting for Switcharoo")
        self.switcharoo.wait_ready()

        self.log("Setting up Switcharoo")
        self.switcharoo_controller.setup()

        self.log("Configuring Tofino TG")
        self.tg_hosts.tg_controller.setup(
            broadcast=self.broadcast,
            symmetric=self.symmetric,
            route=self.route,
        )

        self.log("Waiting for pktgen")
        self.tg_hosts.pktgen.wait_launch()

        self.log("Starting experiment")

        self.log(f"Launching KVS server (delay={self.delay_ns:,}ns)")
        self.kvs_server.kill_server()
        self.kvs_server.launch(delay_ns=self.delay_ns)

        self.log("Launching pktgen")
        self.tg_hosts.pktgen.close()
        self.tg_hosts.pktgen.launch(
            nb_flows=self.total_flows,
            traffic_dist=TrafficDist.ZIPF,
            zipf_param=self.zipf_param,
            kvs_mode=True,
            kvs_get_ratio=KVS_GET_RATIO,
        )

        self.log("Launching Switcharoo")
        self.switcharoo.kill_switchd()
        self.switcharoo.launch()

        self.kvs_server.wait_launch()
        self.tg_hosts.pktgen.wait_launch()
        self.switcharoo.wait_ready()

        self.log("Setting up Switcharoo")
        self.switcharoo_controller.setup()

        report = self.find_stable_throughput(
            tg_controller=self.tg_hosts.tg_controller,
            pktgen=self.tg_hosts.pktgen,
            churn=self.churn_fpm,
        )

        tx_Gbps = report.dut_ingress_bps / 1e9
        tx_Mpps = report.dut_ingress_pps / 1e6

        rx_Gbps = report.dut_egress_bps / 1e9
        rx_Mpps = report.dut_egress_pps / 1e6

        print(f"Experiment: {self.name}")
        print(f"TX     {tx_Mpps:12.5f} Mpps {tx_Gbps:12.5f} Gbps")
        print(f"RX     {rx_Mpps:12.5f} Mpps {rx_Gbps:12.5f} Gbps")

        self.tg_hosts.pktgen.close()
        self.switcharoo.kill_switchd()
        self.kvs_server.kill_server()


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    kill_hosts_on_sigint(config)

    log_file = config["logs"]["experiment"]

    tg_hosts = TGHosts(config, use_accelerator=False)

    switcharoo = Switcharoo(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    )

    switcharoo_controller = SwitcharooController(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        log_file=config["logs"]["controller_dut"],
    )

    kvs_server = KVSServer(
        hostname=config["hosts"]["server"],
        repo=config["repo"]["server"],
        pcie_dev=config["devices"]["server"]["dev"],
        log_file=config["logs"]["server"],
    )

    broadcast = config["devices"]["switch_tg"]["dut_ports"]
    symmetric = []
    route = []

    experiment = SwitcharooThroughput(
        name=EXPERIMENT_NAME,
        delay_ns=STORAGE_SERVER_DELAY_NS,
        tg_hosts=tg_hosts,
        switcharoo=switcharoo,
        switcharoo_controller=switcharoo_controller,
        kvs_server=kvs_server,
        broadcast=broadcast,
        symmetric=symmetric,
        route=route,
        total_flows=TOTAL_FLOWS,
        zipf_param=ZIPF_PARAM,
        churn_fpm=CHURN_FPM,
        experiment_log_file=log_file,
    )

    experiment.run()

    tg_hosts.terminate()
    switcharoo.kill_switchd()
    kvs_server.kill_server()


if __name__ == "__main__":
    main()
