#!/usr/bin/env python3

# Throughput sweep for the Meta4 expert baseline.
#
# Meta4 is a monitoring NF: it learns client-server pairs from DNS responses and attributes those
# pairs' traffic to a domain, reflecting every packet back out its ingress port. No server is
# involved, so this is the "echo" methodology (as for HyperLogLog): the TG floods every DUT port,
# the DUT reflects, and we measure sustained lossless throughput. The traffic is pktgen's DNS mode
# (see deps/pktgen/README.md): pairs announced by DNS responses drawn from the watch list, then
# data server -> client, exactly the workload the meta4 profiles were built from, so the
# synapse-synthesized meta4 can later be measured on the same traffic. The sweep drives the same
# workload knobs as every other throughput experiment, Zipfian flow skew and flow churn (here, the
# rate at which new pairs are announced), repeated for ITERATIONS to get error bars. The DNS
# response ratio is the paper's and stays fixed.
#
# Results are appended to a resumable CSV whose format matches tput_hyperloglog.csv, so the same
# plotting code can consume it. This run takes a long time (len(ZIPF_PARAMS) * len(CHURN_FPM) *
# ITERATIONS stable-throughput searches); it is meant to be left overnight.

import argparse
import filecmp
import tomli
import itertools

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional

from experiments.tput import TGHosts
from experiments.experiment import Experiment, ExperimentTracker
from hosts.meta4 import Meta4, Meta4Controller
from hosts.pktgen import TrafficDist
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

EXPERIMENT_NAME = "Meta4 throughput"
DATA_FILE_NAME = "tput_meta4.csv"

TOTAL_FLOWS = 40_000
CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
ZIPF_PARAMS = [0, 0.2, 0.4, 0.6, 0.8, 1, 1.2]

DNS_RATIO = 0.0014

ITERATIONS = 5

# CHURN_FPM = [1_000_000]
# ZIPF_PARAMS = [1.0]
# ITERATIONS = 1

# The watch list the C NF is built with, and its copy in the expert's directory. The pktgen on the
# remote host reads the former; the expert's controller reads the latter; they have to agree.
NF_DOMAINS_FILE = REPO_DIR / "dpdk-nfs" / "meta4" / "domains.txt"
EXPERT_DOMAINS_FILE = REPO_DIR / "tofino" / "meta4" / "known_domains_v1.txt"


class Meta4Throughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        save_name: Path,
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
        zipf_params: list[float],
        churn_values_fpm: list[int],
        domains: str,
        dns_ratio: float,
        # Logs
        experiment_log_file: Optional[str] = None,
        console: Console = Console(),
    ) -> None:
        super().__init__(name, experiment_log_file, ITERATIONS)

        # Experiment parameters
        self.save_name = save_name

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
        self.zipf_params = zipf_params
        self.churn_values_fpm = churn_values_fpm
        self.domains = domains
        self.dns_ratio = dns_ratio

        self.console = console

        self._sync()

    def _sync(self):
        header = "#it,s,churn (fpm)"
        header += ",requested (bps),pktgen tput (bps),pktgen tput (pps),DUT ingress (bps),DUT ingress (pps),DUT egress (bps),DUT egress (pps)\n"

        self.experiment_tracker = set()
        self.save_name.parent.mkdir(parents=True, exist_ok=True)

        # If file exists, continue where we left off.
        if self.save_name.exists():
            with open(self.save_name) as f:
                read_header = f.readline()
                assert header == read_header
                for row in f.readlines():
                    cols = row.split(",")
                    i = int(cols[0])
                    s = float(cols[1])
                    churn_fpm = int(cols[2])
                    self.experiment_tracker.add((i, s, churn_fpm))
        else:
            with open(self.save_name, "w") as f:
                f.write(header)

    def run(
        self,
        step_progress: Progress,
        current_iter: int,
    ) -> None:
        combinations = list(itertools.product(self.zipf_params, self.churn_values_fpm))
        task_id = step_progress.add_task(f"{self.name} (it={current_iter})", total=len(combinations))

        # Check if we already have everything before running all the programs.
        completed = True
        for s, churn_fpm in combinations:
            exp_key = (
                current_iter,
                s,
                churn_fpm,
            )
            if exp_key not in self.experiment_tracker:
                completed = False
                break
        if completed:
            return

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

        for s, churn_fpm in combinations:
            exp_key = (
                current_iter,
                s,
                churn_fpm,
            )

            description = f"{self.name} (it={current_iter} s={s} churn={churn_fpm:,}fpm)"

            if exp_key in self.experiment_tracker:
                self.console.log(f"[orange1]Skipping: iteration={current_iter} s={s} churn={churn_fpm:,}fpm")
                step_progress.update(task_id, description=description, advance=1)
                continue

            step_progress.update(task_id, description=description)

            self.log("Launching pktgen in DNS mode")
            self.tg_hosts.pktgen.close()
            self.tg_hosts.pktgen.launch(
                nb_flows=self.total_flows,
                traffic_dist=TrafficDist.ZIPF,
                zipf_param=s,
                kvs_mode=False,
                dns_mode=True,
                domains=self.domains,
                dns_ratio=self.dns_ratio,
            )

            self.tg_hosts.pktgen.wait_launch()

            report = self.find_stable_throughput(
                tg_controller=self.tg_hosts.tg_controller,
                pktgen=self.tg_hosts.pktgen,
                churn=churn_fpm,
            )

            with open(self.save_name, "a") as f:
                f.write(f"{current_iter}")
                f.write(f",{s}")
                f.write(f",{churn_fpm}")
                f.write(f",{report.requested_bps}")
                f.write(f",{report.pktgen_bps}")
                f.write(f",{report.pktgen_pps}")
                f.write(f",{report.dut_ingress_bps}")
                f.write(f",{report.dut_ingress_pps}")
                f.write(f",{report.dut_egress_bps}")
                f.write(f",{report.dut_egress_pps}")
                f.write(f"\n")

            step_progress.update(task_id, description=description, advance=1)

        self.tg_hosts.pktgen.close()
        self.meta4.kill_switchd()

        step_progress.update(task_id, visible=False)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    # Both sides must be watching the same names, or the comparison means nothing.
    assert filecmp.cmp(NF_DOMAINS_FILE, EXPERT_DOMAINS_FILE, shallow=False), f"{EXPERT_DOMAINS_FILE} differs from {NF_DOMAINS_FILE}"

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    kill_hosts_on_sigint(config)

    log_file = config["logs"]["experiment"]

    exp_tracker = ExperimentTracker()

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

    exp_tracker.add_experiment(
        Meta4Throughput(
            name=EXPERIMENT_NAME,
            save_name=DATA_DIR / DATA_FILE_NAME,
            tg_hosts=tg_hosts,
            meta4=meta4,
            meta4_controller=meta4_controller,
            broadcast=broadcast,
            symmetric=symmetric,
            route=route,
            total_flows=TOTAL_FLOWS,
            zipf_params=ZIPF_PARAMS,
            churn_values_fpm=CHURN_FPM,
            domains=domains,
            dns_ratio=DNS_RATIO,
            experiment_log_file=log_file,
        )
    )

    exp_tracker.run_experiments()

    tg_hosts.terminate()
    meta4.kill_switchd()


if __name__ == "__main__":
    main()
