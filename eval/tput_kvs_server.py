#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional

from experiments.tput import TGHosts
from experiments.experiment import Experiment, ExperimentTracker
from hosts.kvs_server import KVSServer
from hosts.netcache import NetCache, NetCacheController
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

DELAY_NS_VALUES = [0, 100, 200, 500, 700, 1000, 1200]


class KVSThroughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        save_name: Path,
        delay_values_ns: list[int],
        # Hosts
        tg_hosts: TGHosts,
        netcache: NetCache,
        netcache_controller: NetCacheController,
        kvs_server: KVSServer,
        # TG controller
        broadcast: list[int],
        symmetric: list[int],
        route: list[tuple[int, int]],
        # Pktgen
        experiment_log_file: Optional[str] = None,
        console: Console = Console(),
    ) -> None:
        super().__init__(name, experiment_log_file)

        # Experiment parameters
        self.save_name = save_name
        self.delay_values_ns = delay_values_ns

        # Hosts
        self.tg_hosts = tg_hosts
        self.netcache = netcache
        self.netcache_controller = netcache_controller
        self.kvs_server = kvs_server

        # TG controller
        self.broadcast = broadcast
        self.symmetric = symmetric
        self.route = route

        self.console = console

        self._sync()

    def _sync(self):
        header = "#it, delay (ns), requested (bps), pktgen tput (bps), pktgen tput (pps), DUT ingress (bps), DUT ingress (pps), DUT egress (bps), DUT egress (pps)\n"

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
                    delay_ns = int(cols[1])
                    self.experiment_tracker.add((i, delay_ns))
        else:
            with open(self.save_name, "w") as f:
                f.write(header)

    def run(
        self,
        step_progress: Progress,
        current_iter: int,
    ) -> None:
        task_id = step_progress.add_task(f"{self.name} (it={current_iter})", total=len(self.delay_values_ns))

        # Check if we already have everything before running all the programs.
        completed = True
        for delay_ns in self.delay_values_ns:
            exp_key = (
                current_iter,
                delay_ns,
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
        self.netcache.install()

        self.log("Launching NetCache controller")
        self.netcache_controller.launch(disable_cache=True)

        self.log("Launching pktgen")
        self.tg_hosts.pktgen.launch(kvs_mode=True)

        self.log("Waiting for Tofino TG")
        self.tg_hosts.tg_switch.wait_ready()

        self.log("Configuring Tofino TG")
        self.tg_hosts.tg_controller.setup(
            broadcast=self.broadcast,
            symmetric=self.symmetric,
            route=self.route,
        )

        self.log("Waiting for pktgen")
        self.tg_hosts.pktgen.wait_launch()

        self.log("Starting experiment")

        for delay_ns in self.delay_values_ns:
            exp_key = (
                current_iter,
                delay_ns,
            )

            description = f"{self.name} (it={current_iter} delay={delay_ns:,}ns)"

            if exp_key in self.experiment_tracker:
                self.console.log(f"[orange1]Skipping: iteration {current_iter} delay {delay_ns:,}ns")
                step_progress.update(task_id, description=description, advance=1)
                continue

            step_progress.update(task_id, description=description)

            self.log(f"Launching and waiting for KVS server (delay={delay_ns:,}ns)")
            self.kvs_server.kill_server()
            self.kvs_server.launch(delay_ns=delay_ns)
            self.kvs_server.wait_launch()

            self.log("Waiting for NetCache controller")
            self.netcache_controller.wait_ready()

            self.log("Launching pktgen")
            self.tg_hosts.pktgen.close()
            self.tg_hosts.pktgen.launch(kvs_mode=True)

            self.tg_hosts.pktgen.wait_launch()

            report = self.find_stable_throughput(
                tg_controller=self.tg_hosts.tg_controller,
                pktgen=self.tg_hosts.pktgen,
                churn=0,
            )

            with open(self.save_name, "a") as f:
                f.write(f"{current_iter}")
                f.write(f",{delay_ns}")
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
        self.netcache_controller.kill_controller()
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

    netcache = NetCache(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    )

    netcache_controller = NetCacheController(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        client_ports=config["devices"]["switch_dut"]["client_ports"],
        server_port=config["devices"]["switch_dut"]["server_port"],
        log_file=config["logs"]["controller_dut"],
    )

    kvs_server = KVSServer(
        hostname=config["hosts"]["server"],
        repo=config["repo"]["server"],
        pcie_dev=config["devices"]["server"]["dev"],
        log_file=config["logs"]["server"],
    )

    exp_tracker.add_experiment(
        KVSThroughput(
            name="KVS server tput/delay_ns",
            save_name=DATA_DIR / "tput_kvs_server.csv",
            delay_values_ns=DELAY_NS_VALUES,
            tg_hosts=tg_hosts,
            netcache=netcache,
            netcache_controller=netcache_controller,
            kvs_server=kvs_server,
            broadcast=config["devices"]["switch_tg"]["dut_ports"],
            symmetric=[],
            route=[],
            experiment_log_file=log_file,
        )
    )

    exp_tracker.run_experiments()

    tg_hosts.terminate()
    netcache.kill_switchd()
    netcache_controller.kill_controller()
    kvs_server.kill_server()


if __name__ == "__main__":
    main()
