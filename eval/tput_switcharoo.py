#!/usr/bin/env python3

import argparse
import tomli
import itertools

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional

from experiments.tput import TGHosts
from experiments.experiment import Experiment, ExperimentTracker
from hosts.kvs_server import KVSServer
from hosts.switcharoo import Switcharoo, SwitcharooController
from hosts.pktgen import TrafficDist
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

EXPERIMENT_NAME = "Switcharoo throughput"
DATA_FILE_NAME = "tput_switcharoo.csv"
STORAGE_SERVER_DELAY_NS = 0
TOTAL_FLOWS = 100_000
KVS_GET_RATIO = 0.99
CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
ZIPF_PARAMS = [0, 0.2, 0.4, 0.6, 0.8, 1, 1.2]
ITERATIONS = 3
# CHURN_FPM = [0]
# ZIPF_PARAMS = [1.2]


class SwitcharooThroughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        save_name: Path,
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
        zipf_params: list[float],
        churn_values_fpm: list[int],
        # Logs
        experiment_log_file: Optional[str] = None,
        console: Console = Console(),
    ) -> None:
        super().__init__(name, experiment_log_file, ITERATIONS)

        # Experiment parameters
        self.save_name = save_name
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

        self.log("Installing NetCache")
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

            self.log(f"Launching and waiting for KVS server (delay={self.delay_ns:,}ns)")
            self.kvs_server.kill_server()
            self.kvs_server.launch(delay_ns=self.delay_ns)
            self.kvs_server.wait_launch()

            self.log("Launching pktgen")
            self.tg_hosts.pktgen.close()
            self.tg_hosts.pktgen.launch(
                nb_flows=self.total_flows,
                traffic_dist=TrafficDist.ZIPF,
                zipf_param=s,
                kvs_mode=True,
                kvs_get_ratio=KVS_GET_RATIO,
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
        self.switcharoo.kill_switchd()
        self.kvs_server.kill_server()

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

    exp_tracker.add_experiment(
        SwitcharooThroughput(
            name=EXPERIMENT_NAME,
            save_name=DATA_DIR / DATA_FILE_NAME,
            delay_ns=STORAGE_SERVER_DELAY_NS,
            tg_hosts=tg_hosts,
            switcharoo=switcharoo,
            switcharoo_controller=switcharoo_controller,
            kvs_server=kvs_server,
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
    switcharoo.kill_switchd()
    kvs_server.kill_server()


if __name__ == "__main__":
    main()
