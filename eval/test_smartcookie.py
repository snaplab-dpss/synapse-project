#!/usr/bin/env python3

# Throughput test for the SmartCookie expert baseline.
#
# SmartCookie (Princeton, USENIX Security'24) is a SYN-flooding defense: its
# switch agent answers every TCP SYN with a SYN-ACK carrying a HalfSipHash SYN
# cookie, computed entirely in the data plane (two recirculations per SYN).
# This test reproduces the authors' own benchmark: a SYN flood, measuring the
# lossless SYN-ACK response rate. Every SYN-ACK is reflected to the port the SYN
# came in on, so this is the same "echo" methodology as tput_echo / synapse-echo
# / test_hyperloglog: the TG floods every DUT port, the DUT reflects, and we
# measure sustained lossless throughput. No server is involved: SYNs never
# reach it, and the server agent (eBPF) only matters on the paths a SYN flood
# does not take. pktgen runs in --tcp-syn mode (every packet is a TCP SYN; see
# deps/pktgen/README.md).

import argparse
import tomli

from pathlib import Path

from typing import Optional

from experiments.tput import TGHosts
from experiments.experiment import Experiment
from hosts.smartcookie import SmartCookie, SmartCookieController
from hosts.pktgen import TrafficDist
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

TOTAL_FLOWS = 40_000
CHURN_FPM = 10_000
ZIPF_PARAM = 1.2

EXPERIMENT_NAME = "SmartCookie throughput"


class SmartCookieThroughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        # Hosts
        tg_hosts: TGHosts,
        smartcookie: SmartCookie,
        smartcookie_controller: SmartCookieController,
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

        # Hosts
        self.tg_hosts = tg_hosts
        self.smartcookie = smartcookie
        self.smartcookie_controller = smartcookie_controller

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

        self.log("Installing SmartCookie")
        self.smartcookie.install()

        self.log("Launching SmartCookie")
        self.smartcookie.launch()

        self.log("Launching pktgen")
        self.tg_hosts.pktgen.launch(kvs_mode=False, tcp_syn=True)

        self.log("Waiting for Tofino TG")
        self.tg_hosts.tg_switch.wait_ready()

        self.log("Waiting for SmartCookie")
        self.smartcookie.wait_ready()

        self.log("Setting up SmartCookie (bringing up ports)")
        self.smartcookie_controller.setup()

        self.log("Configuring Tofino TG")
        self.tg_hosts.tg_controller.setup(
            broadcast=self.broadcast,
            symmetric=self.symmetric,
            route=self.route,
        )

        self.log("Waiting for pktgen")
        self.tg_hosts.pktgen.wait_launch()

        self.log("Starting experiment")

        self.log("Launching pktgen (SYN flood)")
        self.tg_hosts.pktgen.close()
        self.tg_hosts.pktgen.launch(
            nb_flows=self.total_flows,
            traffic_dist=TrafficDist.ZIPF,
            zipf_param=self.zipf_param,
            kvs_mode=False,
            tcp_syn=True,
        )

        self.tg_hosts.pktgen.wait_launch()

        report = self.find_stable_throughput(
            tg_controller=self.tg_hosts.tg_controller,
            pktgen=self.tg_hosts.pktgen,
            churn=self.churn_fpm,
        )

        print(report)

        self.tg_hosts.pktgen.close()
        self.smartcookie.kill_switchd()


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    kill_hosts_on_sigint(config)

    log_file = config["logs"]["experiment"]

    tg_hosts = TGHosts(config, use_accelerator=False)

    smartcookie = SmartCookie(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    )

    smartcookie_controller = SmartCookieController(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        log_file=config["logs"]["controller_dut"],
    )

    # Echo methodology: the TG floods every DUT port with SYNs, the DUT reflects
    # a SYN-ACK to each. No paired routing and no server-facing port are involved.
    broadcast = config["devices"]["switch_tg"]["dut_ports"]
    symmetric = []
    route = []

    experiment = SmartCookieThroughput(
        name=EXPERIMENT_NAME,
        tg_hosts=tg_hosts,
        smartcookie=smartcookie,
        smartcookie_controller=smartcookie_controller,
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
    smartcookie.kill_switchd()


if __name__ == "__main__":
    main()
