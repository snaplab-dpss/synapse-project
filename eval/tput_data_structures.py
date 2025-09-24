#!/usr/bin/env python3

import argparse
import tomli
import itertools

from dataclasses import dataclass
from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional

from experiments.tput import ThroughputHosts
from experiments.experiment import Experiment, ExperimentTracker
from hosts.kvs_server import KVSServer
from hosts.pktgen import TrafficDist
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

STORAGE_SERVER_DELAY_NS = 0
KVS_GET_RATIO = 0.99

TOTAL_FLOWS = 40_000
CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2]

# TOTAL_FLOWS = 40_000
# CHURN_FPM = [0]
# ZIPF_PARAMS = [1.2]

ITERATIONS = 3


@dataclass
class NF:
    name: str
    description: str
    data_out: Path
    tofino: Path
    controller: Path
    kvs_mode: bool


NFS = [
    # NF(
    #     name="map_table",
    #     description="MapTable",
    #     data_out=Path("tput_map_table.csv"),
    #     tofino=Path("tofino/data_structures/map_table/map_table.p4"),
    #     controller=Path("tofino/data_structures/map_table/map_table.cpp"),
    #     kvs_mode=False,
    # ),
    # NF(
    #     name="cuckoo_hash_table",
    #     description="CuckooHashTable",
    #     data_out=Path("tput_cuckoo_hash_table.csv"),
    #     tofino=Path("tofino/data_structures/cuckoo_hash_table/cuckoo_hash_table.p4"),
    #     controller=Path("tofino/data_structures/cuckoo_hash_table/cuckoo_hash_table.cpp"),
    #     kvs_mode=True,
    # ),
    *[
        NF(
            name="fcfs_cached_table",
            description="FCFSCachedTable",
            data_out=Path(f"tput_fcfs_cached_table_{cache_capacity}.csv"),
            tofino=Path(f"tofino/data_structures/fcfs_cached_table/fcfs_cached_table_{cache_capacity}.p4"),
            controller=Path("tofino/data_structures/fcfs_cached_table/fcfs_cached_table.cpp"),
            kvs_mode=False,
        )
        for cache_capacity in [4096, 8192, 16384, 32768, 65536]
    ],
]


class Throughput(Experiment):

    def __init__(
        self,
        # Experiment parameters
        name: str,
        save_name: Path,
        # Hosts
        tput_hosts: ThroughputHosts,
        kvs_server: Optional[KVSServer],
        # TG controller
        tg_dut_ports: list[int],
        # NF
        p4_src_in_repo: Path,
        controller_src_in_repo: Path,
        dut_ports: list[int],
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
        self.tput_hosts = tput_hosts
        self.kvs_server = kvs_server

        # TG controller
        self.tg_dut_ports = tg_dut_ports

        # Synapse
        self.p4_src_in_repo = p4_src_in_repo
        self.controller_src_in_repo = controller_src_in_repo
        self.dut_ports = dut_ports

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

        task_id = step_progress.add_task(f"{self.name} (it={current_iter})", total=len(combinations))

        self.log("Installing Tofino TG")
        self.tput_hosts.tg_switch.install()

        self.log("Launching Tofino TG")
        self.tput_hosts.tg_switch.launch()

        self.log("Installing P4 program")
        self.tput_hosts.dut_switch.install(
            src_in_repo=self.p4_src_in_repo,
        )

        self.log("Launching pktgen")
        self.tput_hosts.pktgen.launch()

        self.log("Waiting for Tofino TG")
        self.tput_hosts.tg_switch.wait_ready()

        self.log("Configuring Tofino TG")
        self.tput_hosts.tg_controller.setup(
            broadcast=self.tg_dut_ports,
            symmetric=[],
            route=[],
        )

        self.log("Waiting for pktgen")
        self.tput_hosts.pktgen.wait_launch()

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

            if self.kvs_server:
                self.log(f"Launching KVS server (delay={STORAGE_SERVER_DELAY_NS}ns)")
                self.kvs_server.kill_server()
                self.kvs_server.launch(delay_ns=STORAGE_SERVER_DELAY_NS)
                self.kvs_server.wait_launch()

            self.log("Waiting for controller")
            self.tput_hosts.dut_controller.relaunch_and_wait(
                src_in_repo=self.controller_src_in_repo,
                ports=self.dut_ports,
            )

            self.log("Launching pktgen")
            self.tput_hosts.pktgen.close()
            self.tput_hosts.pktgen.launch(
                nb_flows=self.total_flows,
                traffic_dist=TrafficDist.ZIPF,
                zipf_param=s,
                kvs_mode=self.kvs_server is not None,
                kvs_get_ratio=KVS_GET_RATIO,
            )

            self.tput_hosts.pktgen.wait_launch()

            report = self.find_stable_throughput(
                tg_controller=self.tput_hosts.tg_controller,
                pktgen=self.tput_hosts.pktgen,
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

        self.tput_hosts.pktgen.close()
        self.tput_hosts.dut_controller.stop()
        self.tput_hosts.tg_switch.kill_switchd()

        if self.kvs_server:
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
    dut_ports = config["devices"]["switch_dut"]["client_ports"]
    server_port = config["devices"]["switch_dut"]["server_port"]

    exp_tracker = ExperimentTracker()

    for nf in NFS:
        nf_dut_ports = dut_ports.copy()
        if nf.kvs_mode:
            nf_dut_ports.append(server_port)
            nf_dut_ports = sorted(nf_dut_ports)

        exp_tracker.add_experiment(
            Throughput(
                name=nf.description,
                save_name=DATA_DIR / nf.data_out,
                tput_hosts=tput_hosts,
                kvs_server=kvs_server if nf.kvs_mode else None,
                tg_dut_ports=tg_dut_ports,
                p4_src_in_repo=nf.tofino,
                controller_src_in_repo=nf.controller,
                dut_ports=nf_dut_ports,
                total_flows=TOTAL_FLOWS,
                zipf_params=ZIPF_PARAMS,
                churn_values_fpm=CHURN_FPM,
                experiment_log_file=log_file,
            )
        )

    exp_tracker.run_experiments()

    tput_hosts.terminate()


if __name__ == "__main__":
    main()
