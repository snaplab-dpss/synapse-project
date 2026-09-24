#!/usr/bin/env python3

# Throughput test for the Meta4 expert baseline.
#
# Meta4 is a monitoring NF: it learns client-server pairs from DNS responses and attributes those
# pairs' traffic to a domain, reflecting every packet back out its ingress port. No server is
# involved, so this is the "echo" methodology (as for HyperLogLog): the TG floods every DUT port,
# the DUT reflects, and we measure sustained lossless throughput. The traffic is pktgen's DNS mode
# (see deps/pktgen/README.md): pairs announced by DNS responses drawn from the watch list, then
# data server -> client, exactly the workload the meta4 profiles were built from, so the
# synapse-synthesized meta4 can later be measured on the same traffic.

import argparse
import filecmp
import tomli

from pathlib import Path

from typing import Optional

from experiments.tput import TGHosts
from experiments.experiment import Experiment
from hosts.meta4 import Meta4, Meta4Controller
from hosts.pktgen import TrafficDist
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

TOTAL_FLOWS = 40_000
CHURN_FPM = 1_000_000
ZIPF_PARAM = 1.2

DNS_RATIO = 0.0014

# The watch list the C NF is built with, and its copy in the expert's directory. The pktgen on the
# remote host reads the former; the expert's controller reads the latter; they have to agree.
NF_DOMAINS_FILE = REPO_DIR / "dpdk-nfs" / "meta4" / "domains.txt"
EXPERT_DOMAINS_FILE = REPO_DIR / "tofino" / "meta4" / "known_domains_v1.txt"

EXPERIMENT_NAME = "Meta4 throughput"


class Meta4Throughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        # Hosts
        tg_hosts: TGHosts,
        meta4: Meta4,
        meta4_controller: Meta4Controller,
        # TG controller
        broadcast: list[int],
        symmetric: list[int],
        route: list[tuple[int, int]],
        # Pktgen
        total_flows: int,
        zipf_param: float,
        churn_fpm: int,
        domains: str,
        dns_ratio: float,
        # Logs
        experiment_log_file: Optional[str] = None,
    ) -> None:
        super().__init__(name, experiment_log_file, 1)

        # Hosts
        self.tg_hosts = tg_hosts
        self.meta4 = meta4
        self.meta4_controller = meta4_controller

        # TG controller
        self.broadcast = broadcast
        self.symmetric = symmetric
        self.route = route

        # Pktgen
        self.total_flows = total_flows
        self.zipf_param = zipf_param
        self.churn_fpm = churn_fpm
        self.domains = domains
        self.dns_ratio = dns_ratio

    def run(self) -> None:
        self.log("Installing Tofino TG")
        self.tg_hosts.tg_switch.install()

        self.log("Launching Tofino TG")
        self.tg_hosts.tg_switch.launch()

        self.log("Installing Meta4")
        self.meta4.install()

        self.log("Launching Meta4")
        self.meta4.launch()

        self.log("Launching pktgen")
        self.tg_hosts.pktgen.launch(kvs_mode=False)

        self.log("Waiting for Tofino TG")
        self.tg_hosts.tg_switch.wait_ready()

        self.log("Waiting for Meta4")
        self.meta4.wait_ready()

        self.log("Setting up Meta4 (ports, watch list, client prefixes)")
        self.meta4_controller.setup()

        self.log("Configuring Tofino TG")
        self.tg_hosts.tg_controller.setup(
            broadcast=self.broadcast,
            symmetric=self.symmetric,
            route=self.route,
        )

        self.log("Waiting for pktgen")
        self.tg_hosts.pktgen.wait_launch()

        self.log("Starting experiment")

        self.log("Launching pktgen in DNS mode")
        self.tg_hosts.pktgen.close()
        self.tg_hosts.pktgen.launch(
            nb_flows=self.total_flows,
            traffic_dist=TrafficDist.ZIPF,
            zipf_param=self.zipf_param,
            kvs_mode=False,
            dns_mode=True,
            domains=self.domains,
            dns_ratio=self.dns_ratio,
        )

        self.tg_hosts.pktgen.wait_launch()

        report = self.find_stable_throughput(
            tg_controller=self.tg_hosts.tg_controller,
            pktgen=self.tg_hosts.pktgen,
            churn=self.churn_fpm,
        )

        print(report)

        self.tg_hosts.pktgen.close()
        self.meta4.kill_switchd()


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")
    parser.add_argument("--churn", type=int, default=CHURN_FPM, help="Churn (fpm)")
    parser.add_argument("--zipf-param", type=float, default=ZIPF_PARAM, help="Zipf parameter (0 = uniform)")
    parser.add_argument("--dns-ratio", type=float, default=DNS_RATIO, help="Fraction of packets that are DNS responses")

    args = parser.parse_args()

    # Both sides must be watching the same names, or the comparison means nothing.
    assert filecmp.cmp(NF_DOMAINS_FILE, EXPERT_DOMAINS_FILE, shallow=False), f"{EXPERT_DOMAINS_FILE} differs from {NF_DOMAINS_FILE}"

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    kill_hosts_on_sigint(config)

    log_file = config["logs"]["experiment"]

    tg_hosts = TGHosts(config, use_accelerator=False)

    meta4 = Meta4(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    )

    meta4_controller = Meta4Controller(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        log_file=config["logs"]["controller_dut"],
    )

    # Echo methodology: the TG floods every DUT port, the DUT reflects. No paired
    # routing and no server-facing port are involved.
    broadcast = config["devices"]["switch_tg"]["dut_ports"]
    symmetric = []
    route = []

    # The domains file as pktgen sees it, in its own copy of the repo.
    domains = str(Path(config["repo"]["pktgen"]) / "dpdk-nfs" / "meta4" / "domains.txt")

    experiment = Meta4Throughput(
        name=EXPERIMENT_NAME,
        tg_hosts=tg_hosts,
        meta4=meta4,
        meta4_controller=meta4_controller,
        broadcast=broadcast,
        symmetric=symmetric,
        route=route,
        total_flows=TOTAL_FLOWS,
        zipf_param=args.zipf_param,
        churn_fpm=args.churn,
        domains=domains,
        dns_ratio=args.dns_ratio,
        experiment_log_file=log_file,
    )

    experiment.run()

    tg_hosts.terminate()
    meta4.kill_switchd()


if __name__ == "__main__":
    main()
