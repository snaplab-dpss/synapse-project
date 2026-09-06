#!/usr/bin/env python3

# Throughput sweep for the SmartCookie expert baseline.
#
# SmartCookie's switch agent answers every TCP SYN with a SYN-ACK carrying a
# HalfSipHash SYN cookie computed in the data plane (two recirculations per
# SYN). This sweep reproduces the authors' benchmark - a SYN flood, measuring
# the lossless SYN-ACK response rate - under the same workload knobs as every
# other throughput experiment: Zipfian flow skew and flow churn. The SYN path
# keeps no per-flow state, so the result is expected to be flat across the grid;
# we keep the grid so the heatmap is comparable with the other NFs'. Like
# tput_echo / tput_hyperloglog and unlike the KVS sweeps, NO server is used:
# SYNs never reach it. The TG floods every DUT port with SYNs (pktgen
# --tcp-syn), the DUT reflects a SYN-ACK to each, and we measure sustained
# lossless throughput, repeated for ITERATIONS to get error bars.
#
# Results are appended to a resumable CSV whose format matches
# tput_hyperloglog.csv / tput_switcharoo.csv, so the same plotting code can
# consume it. This run takes a long time (len(ZIPF_PARAMS) * len(CHURN_FPM) *
# ITERATIONS stable-throughput searches); it is meant to be left overnight.

import argparse
import tomli
import itertools

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional

from experiments.tput import TGHosts
from experiments.experiment import Experiment, ExperimentTracker
from hosts.smartcookie import SmartCookie, SmartCookieController
from hosts.pktgen import TrafficDist
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

EXPERIMENT_NAME = "SmartCookie throughput"
DATA_FILE_NAME = "tput_smartcookie.csv"

TOTAL_FLOWS = 40_000
CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
ZIPF_PARAMS = [0, 0.2, 0.4, 0.6, 0.8, 1, 1.2]

ITERATIONS = 5

# CHURN_FPM = [1_000_000]
# ZIPF_PARAMS = [1.0]
# ITERATIONS = 1


class SmartCookieThroughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        save_name: Path,
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
        zipf_params: list[float],
        churn_values_fpm: list[int],
        # Logs
        experiment_log_file: Optional[str] = None,
        console: Console = Console(),
    ) -> None:
        super().__init__(name, experiment_log_file, ITERATIONS)

        # Experiment parameters
        self.save_name = save_name

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
        self.zipf_params = zipf_params
        self.churn_values_fpm = churn_values_fpm

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

            self.log("Launching pktgen (SYN flood)")
            self.tg_hosts.pktgen.close()
            self.tg_hosts.pktgen.launch(
                nb_flows=self.total_flows,
                traffic_dist=TrafficDist.ZIPF,
                zipf_param=s,
                kvs_mode=False,
                tcp_syn=True,
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
        self.smartcookie.kill_switchd()

        step_progress.update(task_id, visible=False)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    kill_hosts_on_sigint(config)

    log_file = config["logs"]["experiment"]

    exp_tracker = ExperimentTracker()

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

    exp_tracker.add_experiment(
        SmartCookieThroughput(
            name=EXPERIMENT_NAME,
            save_name=DATA_DIR / DATA_FILE_NAME,
            tg_hosts=tg_hosts,
            smartcookie=smartcookie,
            smartcookie_controller=smartcookie_controller,
            broadcast=broadcast,
            symmetric=symmetric,
            route=route,
            total_flows=TOTAL_FLOWS,
            zipf_params=ZIPF_PARAMS,
            churn_values_fpm=CHURN_FPM,
            experiment_log_file=log_file,
        )
    )

    exp_tracker.run_experiments()

    tg_hosts.terminate()
    smartcookie.kill_switchd()


if __name__ == "__main__":
    main()
