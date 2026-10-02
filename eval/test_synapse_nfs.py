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
PIPELINES = 1

# Launch the Synapse controller's debug build, which logs what it does with every packet sent
# up to it (in the controller's log). Slower and far more verbose, so it is off for real runs.
DEBUG_MODE = False

TOTAL_FLOWS = 40_000
CHURN_FPM = 10_000
ZIPF_PARAM = 1.0

# Meta4's traffic is pktgen's DNS mode (see deps/pktgen/README.md): pairs announced by DNS
# responses drawn from the watch list the C NF was built with, then data server -> client, the
# workload its profiles were built from. The DNS response ratio is the paper's.
DNS_RATIO = 0.0014
DOMAINS_FILE_IN_REPO = Path("dpdk-nfs/meta4/domains.txt")


@dataclass
class SynapseNF:
    name: str
    description: str
    kvs_mode: bool
    tcp_syn: bool
    tofino: Path
    controller: Path
    broadcast: Callable[[list[int]], list[int]]
    symmetric: Callable[[list[int]], list[int]]
    route: Callable[[list[int]], list[tuple[int, int]]]
    # Meta4's traffic, DNS responses then data; the domains file is read on the pktgen host.
    dns_mode: bool = False


def build_synapse_nf_name(nf: str, churn: int, zipf: float) -> str:
    dist = f"{'unif' if zipf == 0.0 else 'zipf'}{str(int(zipf) if int(zipf) == zipf else zipf).replace('.', '_') if zipf != 0.0 else ''}"
    heuristic = "max-tput"
    return f"{nf}-f{TOTAL_FLOWS}-c{churn}-{dist}-h{heuristic}"


def nf_key(nf: SynapseNF) -> str:
    """What --nfs selects by: a gallium program's name, or the NF a synapse solution was synthesized for."""
    return nf.name if nf.name.startswith("gallium-") else nf.name.split("-f")[0]


def nfs(churn_fpm: int, zipf_param: float) -> list[SynapseNF]:
    """The programs under test: each synapse one is the solution synthesized for this workload."""
    CHURN_FPM = churn_fpm
    ZIPF_PARAM = zipf_param
    return [
        SynapseNF(
            name="gallium-kvs",
            description="Gallium KVS",
            kvs_mode=True,
            tcp_syn=False,
            tofino=Path("synthesized/gallium-kvs.p4"),
            controller=Path("synthesized/gallium-kvs.cpp"),
            broadcast=lambda ports: ports,
            symmetric=lambda _: [],
            route=lambda _: [],
        ),
        SynapseNF(
            name="gallium-fw",
            description="Gallium FW",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path("synthesized/gallium-fw.p4"),
            controller=Path("synthesized/gallium-fw.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name="gallium-nat",
            description="Gallium NAT",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path("synthesized/gallium-nat.p4"),
            controller=Path("synthesized/gallium-nat.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name="gallium-meta4",
            description="Gallium Meta4",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path("synthesized/gallium-meta4.p4"),
            controller=Path("synthesized/gallium-meta4.cpp"),
            broadcast=lambda ports: ports,
            symmetric=lambda _: [],
            route=lambda _: [],
            dns_mode=True,
        ),
        SynapseNF(
            name="gallium-cl",
            description="Gallium CL",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path("synthesized/gallium-cl.p4"),
            controller=Path("synthesized/gallium-cl.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name="gallium-pol",
            description="Gallium Policer",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path("synthesized/gallium-pol.p4"),
            controller=Path("synthesized/gallium-pol.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name="gallium-hyperloglog",
            description="Gallium HyperLogLog",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path("synthesized/gallium-hyperloglog.p4"),
            controller=Path("synthesized/gallium-hyperloglog.cpp"),
            broadcast=lambda ports: ports,
            symmetric=lambda _: [],
            route=lambda _: [],
        ),
        SynapseNF(
            name="gallium-hhh",
            description="Gallium HHH",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path("synthesized/gallium-hhh.p4"),
            controller=Path("synthesized/gallium-hhh.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name=build_synapse_nf_name("kvs", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('kvs', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=True,
            tcp_syn=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('kvs', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('kvs', CHURN_FPM, ZIPF_PARAM)}.cpp"),
            broadcast=lambda ports: ports,
            symmetric=lambda _: [],
            route=lambda _: [],
        ),
        SynapseNF(
            name=build_synapse_nf_name("fw", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('fw', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('fw', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('fw', CHURN_FPM, ZIPF_PARAM)}.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name=build_synapse_nf_name("nat", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('nat', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('nat', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('nat', CHURN_FPM, ZIPF_PARAM)}.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name=build_synapse_nf_name("psd", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('psd', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('psd', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('psd', CHURN_FPM, ZIPF_PARAM)}.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name=build_synapse_nf_name("cl", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('cl', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('cl', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('cl', CHURN_FPM, ZIPF_PARAM)}.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name=build_synapse_nf_name("pol", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('pol', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('pol', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('pol', CHURN_FPM, ZIPF_PARAM)}.cpp"),
            broadcast=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 0],
            symmetric=lambda ports: [p for i, p in enumerate(ports) if i % 2 == 1],
            route=lambda _: [],
        ),
        SynapseNF(
            name=build_synapse_nf_name("hyperloglog", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('hyperloglog', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('hyperloglog', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('hyperloglog', CHURN_FPM, ZIPF_PARAM)}.cpp"),
            broadcast=lambda ports: ports,
            symmetric=lambda _: [],
            route=lambda _: [],
        ),
        SynapseNF(
            name=build_synapse_nf_name("smartcookie", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('smartcookie', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=False,
            tcp_syn=True,
            tofino=Path(f"synthesized/{build_synapse_nf_name('smartcookie', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('smartcookie', CHURN_FPM, ZIPF_PARAM)}.cpp"),
            broadcast=lambda ports: ports,
            symmetric=lambda _: [],
            route=lambda _: [],
        ),
        SynapseNF(
            name=build_synapse_nf_name("meta4", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('meta4', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('meta4', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('meta4', CHURN_FPM, ZIPF_PARAM)}.cpp"),
            broadcast=lambda ports: ports,
            symmetric=lambda _: [],
            route=lambda _: [],
            dns_mode=True,
        ),
        SynapseNF(
            name=build_synapse_nf_name("hhh", CHURN_FPM, ZIPF_PARAM),
            description=f"Synapse {build_synapse_nf_name('hhh', CHURN_FPM, ZIPF_PARAM)}",
            kvs_mode=False,
            tcp_syn=False,
            tofino=Path(f"synthesized/{build_synapse_nf_name('hhh', CHURN_FPM, ZIPF_PARAM)}.p4"),
            controller=Path(f"synthesized/{build_synapse_nf_name('hhh', CHURN_FPM, ZIPF_PARAM)}.cpp"),
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
        tcp_syn: bool,
        dns_mode: bool,
        # Synapse
        p4_src_in_repo: Path,
        controller_src_in_repo: Path,
        dut_ports: list[int],
        # Pktgen
        total_flows: int,
        zipf_param: float,
        churn_fpm: int,
        domains: str,
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
        self.tcp_syn = tcp_syn
        self.dns_mode = dns_mode

        # Synapse
        self.p4_src_in_repo = p4_src_in_repo
        self.controller_src_in_repo = controller_src_in_repo
        self.dut_ports = dut_ports

        # Pktgen
        self.total_flows = total_flows
        self.zipf_param = zipf_param
        self.churn_fpm = churn_fpm
        self.domains = domains

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

        self.log("Waiting for Tofino TG")
        self.tput_hosts.tg_switch.wait_ready()

        self.log("Configuring Tofino TG")
        self.tput_hosts.tg_controller.setup(
            broadcast=self.broadcast,
            symmetric=self.symmetric,
            route=self.route,
        )

        self.log("Starting experiment")

        if self.kvs_mode:
            assert self.kvs_server is not None
            self.log(f"Launching and waiting for KVS server (delay={self.delay_ns:,}ns)")
            self.kvs_server.kill_server()
            self.kvs_server.launch(delay_ns=self.delay_ns)
            self.kvs_server.wait_launch()

        self.log("Launching pktgen")
        self.tput_hosts.pktgen.launch(
            nb_flows=int(self.total_flows / PIPELINES),
            traffic_dist=TrafficDist.ZIPF,
            zipf_param=self.zipf_param,
            kvs_mode=self.kvs_mode,
            kvs_get_ratio=KVS_GET_RATIO,
            tcp_syn=self.tcp_syn,
            dns_mode=self.dns_mode,
            domains=self.domains,
            dns_ratio=DNS_RATIO,
        )

        self.log("Waiting for the Synapse controller")
        self.tput_hosts.dut_controller.wait_ready()

        self.tput_hosts.pktgen.wait_launch()

        report = self.find_stable_throughput(
            tg_controller=self.tput_hosts.tg_controller,
            pktgen=self.tput_hosts.pktgen,
            churn=int(self.churn_fpm / PIPELINES),
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
    parser.add_argument("--churn", type=int, default=CHURN_FPM, help="Churn (fpm); a synapse NF runs the solution synthesized for it")
    parser.add_argument("--zipf-param", type=float, default=ZIPF_PARAM, help="Zipf parameter (0 = uniform); likewise")
    parser.add_argument("--nfs", nargs="+", required=True, choices=[nf_key(nf) for nf in nfs(CHURN_FPM, ZIPF_PARAM)], help="The programs to test (a synapse NF runs the solution synthesized for --churn/--zipf-param)")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    kill_hosts_on_sigint(config)

    log_file = config["logs"]["experiment"]

    tput_hosts = ThroughputHosts(
        config,
        use_accelerator=False,
        debug=DEBUG_MODE,
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

    # The domains file as pktgen sees it, in its own copy of the repo.
    domains = str(Path(config["repo"]["pktgen"]) / DOMAINS_FILE_IN_REPO)

    for nf in [nf for nf in nfs(args.churn, args.zipf_param) if nf_key(nf) in args.nfs]:
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
            tcp_syn=nf.tcp_syn,
            dns_mode=nf.dns_mode,
            p4_src_in_repo=nf.tofino,
            controller_src_in_repo=nf.controller,
            dut_ports=dut_ports,
            total_flows=TOTAL_FLOWS,
            zipf_param=args.zipf_param,
            churn_fpm=args.churn,
            domains=domains,
            experiment_log_file=log_file,
        )

        experiment.run()

    tput_hosts.terminate()
    kvs_server.kill_server()


if __name__ == "__main__":
    main()
