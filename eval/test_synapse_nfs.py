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

STORAGE_SERVER_DELAY_NS = 0
KVS_GET_RATIO = 0.99

TOTAL_FLOWS = 40_000
CHURN_FPM = 10_000
ZIPF_PARAM = 0


@dataclass
class SynapseNF:
    name: str
    description: str
    kvs_mode: bool
    tofino: Path
    controller: Path
    broadcast: Callable[[list[int]], list[int]]
    symmetric: Callable[[list[int]], list[int]]
    route: Callable[[list[int]], list[tuple[int, int]]]


NFS = [
    # SynapseNF(
    #     name="echo",
    #     description="Synapse echo",
    #     kvs_mode=False,
    #     tofino=Path("synthesized/synapse-echo.p4"),
    #     controller=Path("synthesized/synapse-echo.cpp"),
    #     broadcast=lambda ports: ports,
    #     symmetric=lambda _: [],
    #     route=lambda _: [],
    # ),
    # SynapseNF(
    #     name="fwd",
    #     description="Synapse forwarder",
    #     kvs_mode=False,
    #     tofino=Path("synthesized/synapse-fwd.p4"),
    #     controller=Path("synthesized/synapse-fwd.cpp"),
    #     broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
    #     symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
    #     route=lambda _: [],
    # ),
    # SynapseNF(
    #     name="synapse-kvs-hhtable",
    #     description="Synapse KVS HHTable",
    #     kvs_mode=True,
    #     tofino=Path("synthesized/synapse-kvs-hhtable.p4"),
    #     controller=Path("synthesized/synapse-kvs-hhtable.cpp"),
    #     broadcast=lambda ports: ports,
    #     symmetric=lambda _: [],
    #     route=lambda _: [],
    # ),
    # SynapseNF(
    #     name="synapse-kvs-maptable",
    #     description="Synapse KVS MapTable",
    #     kvs_mode=True,
    #     tofino=Path("synthesized/synapse-kvs-maptable.p4"),
    #     controller=Path("synthesized/synapse-kvs-maptable.cpp"),
    #     broadcast=lambda ports: ports,
    #     symmetric=lambda _: [],
    #     route=lambda _: [],
    # ),
    # SynapseNF(
    #     name="synapse-kvs-guardedmaptable",
    #     description="Synapse KVS GuardedMapTable",
    #     kvs_mode=True,
    #     tofino=Path("synthesized/synapse-kvs-guardedmaptable.p4"),
    #     controller=Path("synthesized/synapse-kvs-guardedmaptable.cpp"),
    #     broadcast=lambda ports: ports,
    #     symmetric=lambda _: [],
    #     route=lambda _: [],
    # ),
    SynapseNF(
        name="synapse-fw",
        description="Synapse FW",
        kvs_mode=False,
        tofino=Path("synthesized/synapse-fw.p4"),
        controller=Path("synthesized/synapse-fw.cpp"),
        broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
        symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
        route=lambda _: [],
    ),
]


class Test(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
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
        zipf_param: float,
        churn_fpm: int,
        # Logs
        experiment_log_file: Optional[str] = None,
    ) -> None:
        super().__init__(name, experiment_log_file, 1)

        # Experiment parameters
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
        self.zipf_param = zipf_param
        self.churn_fpm = churn_fpm

        assert not self.kvs_mode or (self.kvs_server is not None)

    def run(self) -> None:
        self.log("Installing Tofino TG")
        self.tput_hosts.tg_switch.install()

        self.log("Launching Tofino TG")
        self.tput_hosts.tg_switch.launch()

        self.log("Installing Synapse P4 program")
        self.tput_hosts.dut_switch.install(
            src_in_repo=self.p4_src_in_repo,
        )

        self.log("Launching Synapse controller")
        self.tput_hosts.dut_controller.launch(
            src_in_repo=self.controller_src_in_repo,
            ports=self.dut_ports,
        )

        self.log("Launching pktgen")
        self.tput_hosts.pktgen.launch(kvs_mode=self.kvs_mode)

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

        if self.kvs_mode:
            assert self.kvs_server is not None
            self.log(f"Launching and waiting for KVS server (delay={self.delay_ns:,}ns)")
            self.kvs_server.kill_server()
            self.kvs_server.launch(delay_ns=self.delay_ns)
            self.kvs_server.wait_launch()

        self.log("Waiting for the Synapse controller")
        self.tput_hosts.dut_controller.wait_ready()

        self.log("Launching pktgen")
        self.tput_hosts.pktgen.close()
        self.tput_hosts.pktgen.launch(
            nb_flows=self.total_flows,
            traffic_dist=TrafficDist.ZIPF,
            zipf_param=self.zipf_param,
            kvs_mode=True,
            kvs_get_ratio=KVS_GET_RATIO,
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

        if self.kvs_mode:
            assert self.kvs_server is not None
            self.kvs_server.kill_server()


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

    for nf in NFS:
        broadcast = nf.broadcast(tg_dut_ports)
        symmetric = nf.symmetric(tg_dut_ports)
        route = nf.route(tg_dut_ports)

        # Force a copy of the list to avoid modifying the original list.
        dut_ports = list(config["devices"]["switch_dut"]["client_ports"])
        if nf.kvs_mode:
            server_port = config["devices"]["switch_dut"]["server_port"]
            dut_ports.append(server_port)
            dut_ports = sorted(dut_ports)

        experiment = Test(
            name=nf.description,
            delay_ns=STORAGE_SERVER_DELAY_NS,
            tput_hosts=tput_hosts,
            kvs_server=kvs_server if nf.kvs_mode else None,
            broadcast=broadcast,
            symmetric=symmetric,
            route=route,
            kvs_mode=nf.kvs_mode,
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
    kvs_server.kill_server()


if __name__ == "__main__":
    main()
