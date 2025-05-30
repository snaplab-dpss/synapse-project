#!/usr/bin/env python3

import os
import sys
import time
import humanize
import asyncio
import rich

from asyncio.subprocess import PIPE, STDOUT
from datetime import timedelta
from dataclasses import dataclass
from argparse import ArgumentParser
from pathlib import Path
from typing import Optional, Tuple
from itertools import product

from orchestrator import Orchestrator, Task

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))

BDD_DIR = (CURRENT_DIR / ".." / "bdds").resolve()
SYNTHESIZED_DIR = (CURRENT_DIR / ".." / "synthesized").resolve()
PCAP_DIR = (CURRENT_DIR / ".." / "pcaps").resolve()
TOOLS_DIR = (CURRENT_DIR / ".." / "tools").resolve()
PROFILE_DIR = (CURRENT_DIR / ".." / "profiles").resolve()
SYNAPSE_DIR = (CURRENT_DIR / ".." / "synapse").resolve()
SYNAPSE_BUILD_DIR = (SYNAPSE_DIR / "build").resolve()
SYNAPSE_BIN_DIR = (SYNAPSE_BUILD_DIR / "bin").resolve()

DEVICES = list(range(2, 32))


@dataclass
class NF:
    name: str
    bdd: str
    pcap_generator: str
    clients: list[int]
    fwd_rules: list[Tuple[int, int]]

    def get_pcap_generator(self) -> Path:
        return SYNAPSE_BIN_DIR / self.pcap_generator


def clients_is_every_other_dev():
    return [c for c in DEVICES if c % 2 == 0]


def connect_every_other_dev():
    return [(i, i + 1) for i in DEVICES if i % 2 == 0]


NFs = {
    "echo": NF("echo", "echo.bdd", "pcap-generator-echo", clients=[], fwd_rules=[]),
    "fwd": NF("fwd", "fwd.bdd", "pcap-generator-fwd", clients=clients_is_every_other_dev(), fwd_rules=connect_every_other_dev()),
    "fw": NF("fw", "fw.bdd", "pcap-generator-fw", clients=clients_is_every_other_dev(), fwd_rules=connect_every_other_dev()),
    "nat": NF("nat", "nat.bdd", "pcap-generator-nat", clients=clients_is_every_other_dev(), fwd_rules=connect_every_other_dev()),
    "kvs": NF("kvs", "kvs.bdd", "pcap-generator-kvs", clients=DEVICES, fwd_rules=[]),
    "cl": NF("cl", "cl.bdd", "pcap-generator-cl", clients=clients_is_every_other_dev(), fwd_rules=connect_every_other_dev()),
}

DEFAULT_NFS = ["echo", "fwd", "fw", "nat", "kvs"]
DEFAULT_TOTAL_PACKETS = [50_000_000]
DEFAULT_PACKET_SIZE = [250]
DEFAULT_TOTAL_FLOWS = [50_000]
DEFAULT_CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
DEFAULT_ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]


def panic(msg: str):
    rich.print(f"ERROR: {msg}")
    exit(1)


def assert_bdd(nf: Path):
    if not nf.is_file():
        panic(f'BDD "{nf}" not found')
    if not nf.name.endswith(".bdd"):
        panic(f'BDD "{nf}" is not a BDD file')


def get_pcap_base_name(
    nf: NF,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
) -> str:
    pcap = f"{nf.name}-f{total_flows}-c{churn_fpm}-{'unif' if zipf_param == 0.0 else 'zipf'}"
    if zipf_param != 0.0:
        s = str(int(zipf_param) if int(zipf_param) == zipf_param else zipf_param).replace(".", "_")
        pcap += s
    return pcap


def build_synapse(
    nfs: list[NF],
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    cmd = "./build.sh"

    files_consumed = [SYNAPSE_DIR / "build.sh"]
    files_produced = [nf.get_pcap_generator() for nf in nfs] + [
        SYNAPSE_BIN_DIR / "bdd-synthesizer",
        SYNAPSE_BIN_DIR / "bdd-visualizer",
    ]

    return Task(
        "build_synapse",
        cmd,
        cwd=SYNAPSE_DIR,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def generate_pcaps(
    nf: NF,
    total_packets: int,
    packet_size: int,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    files_consumed = [nf.get_pcap_generator()]
    files_produced = []

    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)
    for warmup_dev in nf.clients:
        warmup_pcap = f"{pcap_base_name}-dev{warmup_dev}-warmup.pcap"
        files_produced.append(PCAP_DIR / warmup_pcap)
    for dev in DEVICES:
        pcap = f"{pcap_base_name}-dev{dev}.pcap"
        files_produced.append(PCAP_DIR / pcap)

    if nf.fwd_rules:
        dev_list = " ".join([f"{src},{dst}" for src, dst in nf.fwd_rules])
    else:
        dev_list = " ".join([f"{dev}" for dev in DEVICES])

    cmd = f"{nf.get_pcap_generator()}"
    cmd += f" --out {PCAP_DIR}"
    cmd += f" --packets {total_packets}"
    cmd += f" --flows {total_flows}"
    if nf != NFs["kvs"]:
        cmd += f" --packet-size {packet_size}"
    cmd += f" --churn {churn_fpm}"
    if zipf_param == 0.0:
        cmd += " --traffic uniform"
    else:
        cmd += f" --traffic zipf --zipf-param {zipf_param}"
    cmd += f" --devs {dev_list}"
    cmd += " --seed 0"

    return Task(
        f"generate_pcap_{nf.name}_f{total_flows}_c{churn_fpm}_zipf{zipf_param}",
        cmd,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def generate_profiler(
    nf: NF,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    profiler_name = f"{nf.name}-profiler.cpp"

    files_consumed = [BDD_DIR / nf.bdd, SYNAPSE_BIN_DIR / "bdd-synthesizer"]
    files_produced = [SYNTHESIZED_DIR / profiler_name]

    synthesize_profiler_cmd = f"{SYNAPSE_BIN_DIR / 'bdd-synthesizer'}"
    synthesize_profiler_cmd += f" --target profiler"
    synthesize_profiler_cmd += f" --in {BDD_DIR / nf.bdd}"
    synthesize_profiler_cmd += f" --out {SYNTHESIZED_DIR / profiler_name}"

    return Task(
        f"generate_profiler_{nf.name}",
        synthesize_profiler_cmd,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def compile_profiler(
    nf: NF,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
):
    profiler_name = f"{nf.name}-profiler"

    files_consumed = [SYNTHESIZED_DIR / f"{profiler_name}.cpp", TOOLS_DIR / "Makefile.dpdk"]
    files_produced = [SYNTHESIZED_DIR / "build" / profiler_name]

    compile_profiler_cmd = f"make -f {TOOLS_DIR / 'Makefile.dpdk'}"

    return Task(
        f"compile_profiler_{nf.name}",
        compile_profiler_cmd,
        env_vars={"NF": f"{SYNTHESIZED_DIR / profiler_name}.cpp"},
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def profile_nf_against_pcaps(
    nf: NF,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    profiler_name = f"{nf.name}-profiler"
    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)

    report = f"{pcap_base_name}.json"

    warmup_pcaps = []
    for warmup_dev in nf.clients:
        warmup_pcap = f"{pcap_base_name}-dev{warmup_dev}-warmup.pcap"
        warmup_pcaps.append(PCAP_DIR / warmup_pcap)

    pcaps = []
    for dev in DEVICES:
        pcap = f"{pcap_base_name}-dev{dev}.pcap"
        pcaps.append(PCAP_DIR / pcap)

    files_consumed = [SYNTHESIZED_DIR / "build" / profiler_name, *warmup_pcaps, *pcaps]
    files_produced = [PROFILE_DIR / report]

    profile_cmd = f"{SYNTHESIZED_DIR / 'build' / profiler_name}"
    profile_cmd += f" {PROFILE_DIR / report}"
    profile_cmd += " "
    profile_cmd += " ".join([f"--warmup {warmup_dev}:{warmup_pcap}" for warmup_dev, warmup_pcap in zip(nf.clients, warmup_pcaps)])
    profile_cmd += " "
    profile_cmd += " ".join([f"{dev}:{pcap}" for dev, pcap in zip(DEVICES, pcaps)])

    return Task(
        f"profile_nf_against_pcap_{nf.name}_f{total_flows}_c{churn_fpm}_zipf{zipf_param}",
        profile_cmd,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def generate_profile_visualizer(
    nf: NF,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)

    report = f"{pcap_base_name}.json"
    dot = f"{pcap_base_name}.dot"

    files_consumed = [SYNAPSE_BIN_DIR / "bdd-visualizer", BDD_DIR / nf.bdd, PROFILE_DIR / report]
    files_produced = [PROFILE_DIR / dot]

    profile_visualizer_cmd = f"{SYNAPSE_BIN_DIR / 'bdd-visualizer'}"
    profile_visualizer_cmd += f" --in {BDD_DIR / nf.bdd}"
    profile_visualizer_cmd += f" --profile {PROFILE_DIR / report}"
    profile_visualizer_cmd += f" --out {PROFILE_DIR / dot}"

    return Task(
        f"generate_profile_visualizer_{nf.name}_f{total_flows}_c{churn_fpm}_zipf{zipf_param}",
        profile_visualizer_cmd,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def generate_profile_stats(
    nf: NF,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)

    report = f"{pcap_base_name}.json"
    stats = f"{pcap_base_name}.txt"

    files_consumed = [TOOLS_DIR / "profile_stats.py", PROFILE_DIR / report]
    files_produced = [PROFILE_DIR / stats]

    profile_stats_cmd = f"{TOOLS_DIR / 'profile_stats.py'}"
    profile_stats_cmd += f" {PROFILE_DIR / report}"
    profile_stats_cmd += f" --out {PROFILE_DIR / stats}"

    return Task(
        f"generate_profile_stats_{nf.name}_f{total_flows}_c{churn_fpm}_zipf{zipf_param}",
        profile_stats_cmd,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


if __name__ == "__main__":
    description = "Profiler helper script. This will generate the pcaps and profiles for all the possible combinations of the provided parameters."
    description += f" Profiler dir: {PROFILE_DIR}."
    description += f" Pcap dir: {PCAP_DIR}."

    parser = ArgumentParser(description=description)

    parser.add_argument("--nfs", type=str, choices=NFs.keys(), nargs="+", required=True, help="Target NFs to profile")
    parser.add_argument("--total-packets", type=int, nargs="+", default=DEFAULT_TOTAL_PACKETS, help="Total packets to send")
    parser.add_argument("--packet-size", type=int, nargs="+", default=DEFAULT_PACKET_SIZE, help="Packet size (bytes)")
    parser.add_argument("--total-flows", type=int, nargs="+", default=DEFAULT_TOTAL_FLOWS, help="Total flows to generate")
    parser.add_argument("--zipf-params", type=float, nargs="+", default=DEFAULT_ZIPF_PARAMS, help="Zipf parameters")
    parser.add_argument("--churn", type=int, nargs="+", default=DEFAULT_CHURN_FPM, help="Churn rate (fpm)")

    parser.add_argument("--skip-pcap-generation", action="store_true", default=False, help="Skip pcap generation")
    parser.add_argument("--skip-profiler-generation", action="store_true", default=False, help="Skip profiler generation")
    parser.add_argument("--show-cmds-output", action="store_true", default=False, help="Show command output during execution")
    parser.add_argument("--show-cmds", action="store_true", default=False, help="Show requested commands during execution")
    parser.add_argument("--show-execution-plan", action="store_true", default=False, help="Show execution plan")
    parser.add_argument("--dry-run", action="store_true", default=False)
    parser.add_argument("--force", action="store_true", default=False, help="Force execution even if files are already produced")

    parser.add_argument("--silence", action="store_true", default=False, help="Silence all output except errors")

    args = parser.parse_args()

    Path.mkdir(PROFILE_DIR, exist_ok=True)
    Path.mkdir(PCAP_DIR, exist_ok=True)

    combinations = list(
        product(
            args.total_packets,
            args.packet_size,
            args.total_flows,
            args.zipf_params,
            args.churn,
        )
    )

    total_combinations = len(args.nfs) * len(combinations)

    orchestrator = Orchestrator()

    orchestrator.add_task(
        build_synapse(
            nfs=[NFs[nf_name] for nf_name in args.nfs],
            show_cmds_output=args.show_cmds_output,
            show_cmds=args.show_cmds,
            silence=args.silence,
        )
    )

    for nf_name in args.nfs:
        nf = NFs[nf_name]
        bdd = BDD_DIR / nf.bdd
        assert_bdd(bdd)

        orchestrator.add_task(
            generate_profiler(
                nf,
                skip_execution=args.skip_profiler_generation,
                show_cmds_output=args.show_cmds_output,
                show_cmds=args.show_cmds,
                silence=args.silence,
            )
        )

        orchestrator.add_task(
            compile_profiler(
                nf,
                show_cmds_output=args.show_cmds_output,
                show_cmds=args.show_cmds,
                silence=args.silence,
            )
        )

        for total_packets, packet_size, total_flows, zipf_param, churn in combinations:
            orchestrator.add_task(
                generate_pcaps(
                    nf,
                    total_packets,
                    packet_size,
                    total_flows,
                    zipf_param,
                    churn,
                    skip_execution=args.skip_pcap_generation,
                    show_cmds_output=args.show_cmds_output,
                    show_cmds=args.show_cmds,
                    silence=args.silence,
                )
            )

            orchestrator.add_task(
                profile_nf_against_pcaps(
                    nf,
                    total_flows,
                    zipf_param,
                    churn,
                    show_cmds_output=args.show_cmds_output,
                    show_cmds=args.show_cmds,
                    silence=args.silence,
                )
            )

            orchestrator.add_task(
                generate_profile_visualizer(
                    nf,
                    total_flows,
                    zipf_param,
                    churn,
                    show_cmds_output=args.show_cmds_output,
                    show_cmds=args.show_cmds,
                    silence=args.silence,
                )
            )

            orchestrator.add_task(
                generate_profile_stats(
                    nf,
                    total_flows,
                    zipf_param,
                    churn,
                    show_cmds_output=args.show_cmds_output,
                    show_cmds=args.show_cmds,
                    silence=args.silence,
                )
            )

    rich.print("============ Requested Configuration ============")
    rich.print(f"Target NFs:    {args.nfs}")
    rich.print(f"Total packets: {args.total_packets}")
    rich.print(f"Packet sizes:  {args.packet_size}")
    rich.print(f"Total flows:   {args.total_flows}")
    rich.print(f"Zipf params:   {args.zipf_params}")
    rich.print(f"Churn rates:   {args.churn}")
    rich.print("-------------------------------------------------")
    rich.print(f"Skip pcap generation:     {args.skip_pcap_generation}")
    rich.print(f"Skip profiler generation: {args.skip_profiler_generation}")
    rich.print(f"Total combinations:       {total_combinations}")
    rich.print(f"Total tasks:              {orchestrator.size()}")
    rich.print("=================================================")

    if args.show_execution_plan:
        orchestrator.visualize()

    if args.dry_run:
        exit(0)

    orchestrator.run(
        skip_if_already_produced=not args.force,
    )
