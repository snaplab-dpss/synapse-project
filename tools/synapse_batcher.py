#!/usr/bin/env python3

import os
import rich

from dataclasses import dataclass
from argparse import ArgumentParser
from pathlib import Path
from itertools import product

from orchestrator import Orchestrator, Task

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / "..").resolve()

BDD_DIR = PROJECT_DIR / "bdds"
SYNTHESIZED_DIR = PROJECT_DIR / "synthesized"
PCAP_DIR = PROJECT_DIR / "pcaps"
TOOLS_DIR = PROJECT_DIR / "tools"
PROFILE_DIR = PROJECT_DIR / "profiles"
SYNAPSE_DIR = PROJECT_DIR / "synapse"
CONFIGS_DIR = PROJECT_DIR / "configs"

SYNAPSE_BUILD_DIR = SYNAPSE_DIR / "build"
SYNAPSE_BIN_DIR = SYNAPSE_BUILD_DIR / "bin"


@dataclass
class NF:
    name: str
    bdd: str


NFs = {
    "echo": NF("echo", "echo.bdd"),
    "fwd": NF("fwd", "fwd.bdd"),
    "fw": NF("fw", "fw.bdd"),
    "nat": NF("nat", "nat.bdd"),
    "kvs": NF("kvs", "kvs.bdd"),
    "cl": NF("cl", "cl.bdd"),
}

DEFAULT_NFS = ["echo", "fwd", "fw", "nat", "kvs"]
DEFAULT_TOTAL_FLOWS = [25_000]
DEFAULT_CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
DEFAULT_ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0]
DEFAULT_HEURISTICS = ["max-tput", "ds-pref-simple", "ds-pref-guardedmaptable", "ds-pref-hhtable", "ds-pref-cuckoo"]


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
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    cmd = "./build.sh"

    files_consumed = [SYNAPSE_DIR / "build.sh"]
    files_produced = [SYNAPSE_BIN_DIR / "synapse"]

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


def run_synapse(
    nf: NF,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
    heuristic: str,
    seed: int,
    skip_synthesis: bool = True,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)

    profile_report = f"{pcap_base_name}.json"

    name = f"{pcap_base_name}-h{heuristic}"
    synapse_bdd_dot = f"{name}-bdd.dot"
    synapse_ep_dot = f"{name}-ep.dot"
    synapse_ss_dot = f"{name}-ss.dot"
    synapse_report_json = f"{name}.json"

    config = "tofino2-kvs.toml" if nf.name == "kvs" else "tofino2.toml"

    files_consumed = [
        SYNAPSE_BIN_DIR / "synapse",
        BDD_DIR / nf.bdd,
        CONFIGS_DIR / config / PROFILE_DIR / profile_report,
    ]

    files_produced = [
        SYNTHESIZED_DIR / synapse_bdd_dot,
        SYNTHESIZED_DIR / synapse_ep_dot,
        SYNTHESIZED_DIR / synapse_ss_dot,
        SYNTHESIZED_DIR / synapse_report_json,
    ]

    profile_visualizer_cmd = f"{SYNAPSE_BIN_DIR / 'synapse'}"
    profile_visualizer_cmd += f" --in {BDD_DIR / nf.bdd}"
    profile_visualizer_cmd += f" --config {CONFIGS_DIR / config}"
    profile_visualizer_cmd += f" --heuristic {heuristic}"
    profile_visualizer_cmd += f" --seed {seed}"
    profile_visualizer_cmd += f" --profile {PROFILE_DIR / profile_report}"
    profile_visualizer_cmd += f" --name {name}"
    profile_visualizer_cmd += f" --out {SYNTHESIZED_DIR}"
    profile_visualizer_cmd += " --skip-synthesis" if skip_synthesis else ""

    return Task(
        f"run_synapse_{name}",
        profile_visualizer_cmd,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


if __name__ == "__main__":
    description = "Synapse batcher script. This will run synapse against a batch of NFs and profiling reports."
    description += f" Profiler dir: {PROFILE_DIR}."
    description += f" Synthesized dir: {SYNTHESIZED_DIR}."

    parser = ArgumentParser(description=description)

    parser.add_argument("--nfs", type=str, choices=NFs.keys(), nargs="+", required=True, help="Target NFs to profile")
    parser.add_argument("--total-flows", type=int, nargs="+", default=DEFAULT_TOTAL_FLOWS, help="Total flows to generate")
    parser.add_argument("--zipf-params", type=float, nargs="+", default=DEFAULT_ZIPF_PARAMS, help="Zipf parameters")
    parser.add_argument("--churn", type=int, nargs="+", default=DEFAULT_CHURN_FPM, help="Churn rate (fpm)")
    parser.add_argument("--heuristics", type=str, nargs="+", default=DEFAULT_HEURISTICS, help="Heuristic to use for searching")
    parser.add_argument("--seed", type=int, default=0, help="Seed for random number generation")

    parser.add_argument("--synthesize", action="store_true", default=False, help="Synthesize solutions")

    parser.add_argument("--show-cmds-output", action="store_true", default=False, help="Show command output during execution")
    parser.add_argument("--show-cmds", action="store_true", default=False, help="Show requested commands during execution")
    parser.add_argument("--show-execution-plan", action="store_true", default=False, help="Show execution plan")
    parser.add_argument("--dry-run", action="store_true", default=False)
    parser.add_argument("--force", action="store_true", default=False, help="Force execution even if files are already produced")

    parser.add_argument("--silence", action="store_true", default=False, help="Silence all output except errors")

    args = parser.parse_args()

    Path.mkdir(PROFILE_DIR, exist_ok=True)
    Path.mkdir(SYNTHESIZED_DIR, exist_ok=True)

    combinations = list(product(args.total_flows, args.zipf_params, args.churn, args.heuristics))

    total_combinations = len(args.nfs) * len(combinations)

    orchestrator = Orchestrator()

    orchestrator.add_task(
        build_synapse(
            show_cmds_output=args.show_cmds_output,
            show_cmds=args.show_cmds,
            silence=args.silence,
        )
    )

    for nf_name in args.nfs:
        nf = NFs[nf_name]
        bdd = BDD_DIR / nf.bdd
        assert_bdd(bdd)

        for total_flows, zipf_param, churn, heuristic in combinations:
            orchestrator.add_task(
                run_synapse(
                    nf,
                    total_flows,
                    zipf_param,
                    churn,
                    heuristic,
                    args.seed,
                    skip_synthesis=not args.synthesize,
                    show_cmds_output=args.show_cmds_output,
                    show_cmds=args.show_cmds,
                    silence=args.silence,
                )
            )

    rich.print("============ Requested Configuration ============")
    rich.print(f"Target NFs:    {args.nfs}")
    rich.print(f"Total flows:   {args.total_flows}")
    rich.print(f"Zipf params:   {args.zipf_params}")
    rich.print(f"Churn rates:   {args.churn}")
    rich.print(f"Heuristics:    {args.heuristics}")
    rich.print(f"Seed:          {args.seed}")
    rich.print("-------------------------------------------------")
    rich.print(f"Synthesize:         {'Yes' if args.synthesize else 'No'}")
    rich.print(f"Total combinations: {total_combinations}")
    rich.print(f"Total tasks:        {orchestrator.size()}")
    rich.print("=================================================")

    if args.show_execution_plan:
        orchestrator.visualize()

    if args.dry_run:
        exit(0)

    orchestrator.run(
        skip_if_already_produced=not args.force,
    )
