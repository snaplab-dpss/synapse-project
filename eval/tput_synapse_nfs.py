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

STORAGE_SERVER_DELAY_NS = 0
KVS_GET_RATIO = 0.99

TOTAL_FLOWS = 40_000
CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2]

# TOTAL_FLOWS = 40_000
# CHURN_FPM = [0]
# ZIPF_PARAMS = [1.0]

ITERATIONS = 3


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
    churn: list[int]
    zipf: list[float]


def build_synapse_nf_name(nf: str, churn: int, zipf: float) -> str:
    dist = f"{'unif' if zipf == 0.0 else 'zipf'}{str(int(zipf) if int(zipf) == zipf else zipf).replace('.', '_') if zipf != 0.0 else ''}"
    heuristic = "max-tput"
    return f"{nf}-f{TOTAL_FLOWS}-c{churn}-{dist}-h{heuristic}"


SYNAPSE_NFS = [
    # SynapseNF(
    #     name="echo",
    #     description="Synapse echo",
    #     data_out=Path("tput_synapse_echo.csv"),
    #     kvs_mode=False,
    #     tofino=Path("synthesized/synapse-echo.p4"),
    #     controller=Path("synthesized/synapse-echo.cpp"),
    #     broadcast=lambda ports: ports,
    #     symmetric=lambda _: [],
    #     route=lambda _: [],
    #     churn=CHURN_FPM,
    #     zipf=ZIPF_PARAMS,
    # ),
    # SynapseNF(
    #     name="fwd",
    #     description="Synapse forwarder",
    #     data_out=Path("tput_synapse_fwd.csv"),
    #     kvs_mode=False,
    #     tofino=Path("synthesized/synapse-fwd.p4"),
    #     controller=Path("synthesized/synapse-fwd.cpp"),
    #     broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
    #     symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
    #     route=lambda _: [],
    #     churn=CHURN_FPM,
    #     zipf=ZIPF_PARAMS,
    # ),
    # SynapseNF(
    #     name="synapse-kvs-hhtable",
    #     description="Synapse KVS HHTable",
    #     data_out=Path("tput_synapse_kvs_hhtable.csv"),
    #     kvs_mode=True,
    #     tofino=Path("synthesized/synapse-kvs-hhtable.p4"),
    #     controller=Path("synthesized/synapse-kvs-hhtable.cpp"),
    #     broadcast=lambda ports: ports,
    #     symmetric=lambda _: [],
    #     route=lambda _: [],
    #     churn=CHURN_FPM,
    #     zipf=ZIPF_PARAMS,
    # ),
    # SynapseNF(
    #     name="synapse-kvs-cuckoo",
    #     description="Synapse KVS Cuckoo",
    #     data_out=Path("tput_synapse_kvs_cuckoo.csv"),
    #     kvs_mode=True,
    #     tofino=Path("synthesized/synapse-kvs-cuckoo.p4"),
    #     controller=Path("synthesized/synapse-kvs-cuckoo.cpp"),
    #     broadcast=lambda ports: ports,
    #     symmetric=lambda _: [],
    #     route=lambda _: [],
    #     churn=CHURN_FPM,
    #     zipf=ZIPF_PARAMS,
    # ),
    # SynapseNF(
    #     name="synapse-kvs-maptable",
    #     description="Synapse KVS MapTable",
    #     data_out=Path("tput_synapse_kvs_maptable.csv"),
    #     kvs_mode=True,
    #     tofino=Path("synthesized/synapse-kvs-maptable.p4"),
    #     controller=Path("synthesized/synapse-kvs-maptable.cpp"),
    #     broadcast=lambda ports: ports,
    #     symmetric=lambda _: [],
    #     route=lambda _: [],
    #     churn=CHURN_FPM,
    #     zipf=ZIPF_PARAMS,
    # ),
    # SynapseNF(
    #     name="synapse-kvs-guardedmaptable",
    #     description="Synapse KVS GuardedMapTable",
    #     data_out=Path("tput_synapse_kvs_guardedmaptable.csv"),
    #     kvs_mode=True,
    #     tofino=Path("synthesized/synapse-kvs-guardedmaptable.p4"),
    #     controller=Path("synthesized/synapse-kvs-guardedmaptable.cpp"),
    #     broadcast=lambda ports: ports,
    #     symmetric=lambda _: [],
    #     route=lambda _: [],
    #     churn=CHURN_FPM,
    #     zipf=ZIPF_PARAMS,
    # ),
    # SynapseNF(
    #     name="synapse-fw",
    #     description="Synapse FW",
    #     data_out=Path("tput_synapse_fw.csv"),
    #     kvs_mode=False,
    #     tofino=Path("synthesized/synapse-fw.p4"),
    #     controller=Path("synthesized/synapse-fw.cpp"),
    #     broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
    #     symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
    #     route=lambda _: [],
    #     churn=CHURN_FPM,
    #     zipf=ZIPF_PARAMS,
    # ),
    # SynapseNF(
    #     name="synapse-nat",
    #     description="Synapse NAT",
    #     data_out=Path("tput_synapse_nat.csv"),
    #     kvs_mode=False,
    #     tofino=Path("synthesized/synapse-nat.p4"),
    #     controller=Path("synthesized/synapse-nat.cpp"),
    #     broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
    #     symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
    #     route=lambda _: [],
    #     churn=CHURN_FPM,
    #     zipf=ZIPF_PARAMS,
    # ),
    # *[
    #     SynapseNF(
    #         name=build_synapse_nf_name("kvs", churn, s),
    #         description=f"Synapse {build_synapse_nf_name('kvs', churn, s)}",
    #         data_out=Path(f"tput_synapse_kvs.csv"),
    #         kvs_mode=True,
    #         tofino=Path(f"synthesized/{build_synapse_nf_name('kvs', churn, s)}.p4"),
    #         controller=Path(f"synthesized/{build_synapse_nf_name('kvs', churn, s)}.cpp"),
    #         broadcast=lambda ports: ports,
    #         symmetric=lambda _: [],
    #         route=lambda _: [],
    #         churn=[churn],
    #         zipf=[s],
    #     )
    #     for churn, s in itertools.product(CHURN_FPM, ZIPF_PARAMS)
    # ],
    *[
        SynapseNF(
            name=build_synapse_nf_name("fw", churn, s),
            description=f"Synapse {build_synapse_nf_name('fw', churn, s)}",
            data_out=Path(f"tput_synapse_fw.csv"),
            kvs_mode=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('fw', churn, s)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('fw', churn, s)}.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
            churn=[churn],
            zipf=[s],
        )
        for churn, s in itertools.product(CHURN_FPM, ZIPF_PARAMS)
    ],
]


class SynapseThroughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        save_name: Path,
        delay_ns: int,
        # Hosts
        tput_hosts: ThroughputHosts,
        kvs_server: Optional[KVSServer],
        # TG controller
        broadcast: list[int],
        symmetric: list[int],
        route: list[tuple[int, int]],
        kvs_mode: bool,
        # Synapse
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
        self.delay_ns = delay_ns

        # Hosts
        self.tput_hosts = tput_hosts
        self.kvs_server = kvs_server

        # TG controller
        self.broadcast = broadcast
        self.symmetric = symmetric
        self.route = route
        self.kvs_mode = kvs_mode

        # Synapse
        self.p4_src_in_repo = p4_src_in_repo
        self.controller_src_in_repo = controller_src_in_repo
        self.dut_ports = dut_ports

        # Pktgen
        self.total_flows = total_flows
        self.zipf_params = zipf_params
        self.churn_values_fpm = churn_values_fpm

        assert not self.kvs_mode or (self.kvs_server is not None)

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

        self.log("Installing Synapse P4 program")
        self.tput_hosts.dut_switch.install(
            src_in_repo=self.p4_src_in_repo,
        )

        self.log("Waiting for Tofino TG")
        self.tput_hosts.tg_switch.wait_ready()

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

            if self.kvs_mode:
                assert self.kvs_server is not None
                self.log(f"Launching and waiting for KVS server (delay={self.delay_ns:,}ns)")
                self.kvs_server.kill_server()
                self.kvs_server.launch(delay_ns=self.delay_ns)
                self.kvs_server.wait_launch()

            self.log("Configuring Tofino TG")
            self.tput_hosts.tg_controller.setup(
                broadcast=self.broadcast,
                symmetric=self.symmetric,
                route=self.route,
            )

            self.log("Waiting for the Synapse controller")
            self.tput_hosts.dut_controller.stop()
            self.tput_hosts.dut_controller.launch(
                src_in_repo=self.controller_src_in_repo,
                ports=self.dut_ports,
            )

            self.log("Launching pktgen")
            self.tput_hosts.pktgen.close()
            self.tput_hosts.pktgen.launch(
                nb_flows=self.total_flows,
                traffic_dist=TrafficDist.ZIPF,
                zipf_param=s,
                kvs_mode=True,
                kvs_get_ratio=KVS_GET_RATIO,
            )

            self.tput_hosts.dut_controller.wait_ready()
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

        if self.kvs_mode:
            assert self.kvs_server is not None
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
    symmetric = []
    route = []

    exp_tracker = ExperimentTracker()

    for synapse_nf in SYNAPSE_NFS:
        broadcast = synapse_nf.broadcast(tg_dut_ports)
        symmetric = synapse_nf.symmetric(tg_dut_ports)
        route = synapse_nf.route(tg_dut_ports)

        # Force a copy of the list to avoid modifying the original list.
        dut_ports = list(config["devices"]["switch_dut"]["client_ports"])
        if synapse_nf.kvs_mode:
            server_port = config["devices"]["switch_dut"]["server_port"]
            dut_ports.append(server_port)
            dut_ports = sorted(dut_ports)

        exp_tracker.add_experiment(
            SynapseThroughput(
                name=synapse_nf.description,
                save_name=DATA_DIR / synapse_nf.data_out,
                delay_ns=STORAGE_SERVER_DELAY_NS,
                tput_hosts=tput_hosts,
                kvs_server=kvs_server if synapse_nf.kvs_mode else None,
                broadcast=broadcast,
                symmetric=symmetric,
                route=route,
                kvs_mode=synapse_nf.kvs_mode,
                p4_src_in_repo=synapse_nf.tofino,
                controller_src_in_repo=synapse_nf.controller,
                dut_ports=dut_ports,
                total_flows=TOTAL_FLOWS,
                zipf_params=synapse_nf.zipf,
                churn_values_fpm=synapse_nf.churn,
                experiment_log_file=log_file,
            )
        )

    exp_tracker.run_experiments()

    tput_hosts.terminate()
    kvs_server.kill_server()


if __name__ == "__main__":
    main()
