#!/usr/bin/env python3

import os
import rich

from dataclasses import dataclass
from argparse import ArgumentParser
from pathlib import Path
from itertools import product

from helpers.orchestrator import Orchestrator, Task

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / "..").resolve()

BDD_DIR = PROJECT_DIR / "bdds"
SYNTHESIZED_DIR = PROJECT_DIR / "synthesized"
OUT_DIR = SYNTHESIZED_DIR
PCAP_DIR = PROJECT_DIR / "pcaps"
TOFINO_TOOLS_DIR = PROJECT_DIR / "tofino" / "tools"
TOOLS_DIR = PROJECT_DIR / "tools"
PROFILE_DIR = PROJECT_DIR / "profiles"
SYNAPSE_DIR = PROJECT_DIR / "synapse"
CONFIGS_DIR = PROJECT_DIR / "configs"
PLOTS_DIR = PROJECT_DIR / "plots"

SYNAPSE_BUILD_DIR = SYNAPSE_DIR / "build"
SYNAPSE_BIN_DIR = SYNAPSE_BUILD_DIR / "bin"

DEFAULT_NFS = ["echo", "fwd", "fw", "nat", "kvs"]
DEFAULT_TOTAL_FLOWS = [40_000]
DEFAULT_CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
DEFAULT_ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2]
DEFAULT_HEURISTICS = ["max-tput", "ds-pref-simple", "ds-pref-guardedmaptable", "ds-pref-hhtable", "ds-pref-cuckoo"]

REQUIRED_ENV_VARS = ["SDE", "SDE_INSTALL"]


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


def build_synthesized_synapse_nf(
    nf: NF,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
    heuristic: str,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)
    name = f"{pcap_base_name}-h{heuristic}"

    cpp_file = f"{name}.cpp"
    p4_file = f"{name}.p4"

    files_consumed = [
        OUT_DIR / cpp_file,
        OUT_DIR / p4_file,
    ]

    files_produced = [
        OUT_DIR / "build" / "debug" / name,
        OUT_DIR / "build" / "release" / name,
    ]

    build_synapse_nf_cmd = f"{TOFINO_TOOLS_DIR / 'build_synapse_nf.sh'} {name}"

    return Task(
        f"build_synapse_nf_{name}",
        build_synapse_nf_cmd,
        env_vars={"SDE": os.environ["SDE"], "SDE_INSTALL": os.environ["SDE_INSTALL"]},
        cwd=SYNTHESIZED_DIR,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


if __name__ == "__main__":
    for required_env_var in REQUIRED_ENV_VARS:
        if required_env_var not in os.environ:
            panic(f"Required environment variable '{required_env_var}' not set")

    description = "Synapse batcher script. This will run synapse against a batch of NFs and profiling reports."
    description += f" Profiler dir: {PROFILE_DIR}."
    description += f" Synthesized dir: {SYNTHESIZED_DIR}."

    parser = ArgumentParser(description=description)

    parser.add_argument("--nfs", type=str, choices=NFs.keys(), nargs="+", required=True, help="Target NFs to profile")
    parser.add_argument("--total-flows", type=int, nargs="+", default=DEFAULT_TOTAL_FLOWS, help="Total flows to generate")
    parser.add_argument("--zipf-params", type=float, nargs="+", default=DEFAULT_ZIPF_PARAMS, help="Zipf parameters")
    parser.add_argument("--churns", type=int, nargs="+", default=DEFAULT_CHURN_FPM, help="Churn rate (fpm)")
    parser.add_argument("--heuristics", type=str, nargs="+", default=DEFAULT_HEURISTICS, help="Heuristic to use for searching")

    parser.add_argument("--show-cmds-output", action="store_true", default=False, help="Show command output during execution")
    parser.add_argument("--show-cmds", action="store_true", default=False, help="Show requested commands during execution")
    parser.add_argument("--show-execution-plan", action="store_true", default=False, help="Show execution plan")
    parser.add_argument("--dry-run", action="store_true", default=False)
    parser.add_argument("--force", action="store_true", default=False, help="Force execution even if files are already produced")

    parser.add_argument("--silence", action="store_true", default=False, help="Silence all output except errors")

    args = parser.parse_args()

    Path.mkdir(PROFILE_DIR, exist_ok=True)
    Path.mkdir(SYNTHESIZED_DIR, exist_ok=True)

    nfs = [NFs[nf_name] for nf_name in args.nfs]
    combinations = list(product(args.total_flows, args.zipf_params, args.churns, args.heuristics))
    total_combinations = len(args.nfs) * len(combinations)

    orchestrator = Orchestrator()

    for nf in nfs:
        for total_flows, zipf_param, churn, heuristic in combinations:
            orchestrator.add_task(
                build_synthesized_synapse_nf(
                    nf,
                    total_flows,
                    zipf_param,
                    churn,
                    heuristic,
                    show_cmds_output=args.show_cmds_output,
                    show_cmds=args.show_cmds,
                    silence=args.silence,
                )
            )

    rich.print("============ Requested Configuration ============")
    rich.print(f"Target NFs:    {args.nfs}")
    rich.print(f"Total flows:   {args.total_flows}")
    rich.print(f"Zipf params:   {args.zipf_params}")
    rich.print(f"Churn rates:   {args.churns}")
    rich.print(f"Heuristics:    {args.heuristics}")
    rich.print("-------------------------------------------------")
    rich.print(f"Total combinations: {total_combinations}")
    rich.print(f"Total tasks:        {orchestrator.size()}")
    rich.print("=================================================")

    if args.show_execution_plan:
        orchestrator.visualize()

    orchestrator.run(
        skip_if_already_produced=not args.force,
    )
