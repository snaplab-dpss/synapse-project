#!/usr/bin/env python3

import os
import rich

from dataclasses import dataclass
from argparse import ArgumentParser
from pathlib import Path

from helpers.orchestrator import Orchestrator, Task

CURRENT_DIR = Path(os.path.abspath(os.path.dirname(__file__)))
PROJECT_DIR = (CURRENT_DIR / "..").resolve()

BDD_DIR = PROJECT_DIR / "bdds"
NFS_DIR = PROJECT_DIR / "dpdk-nfs"
SYNAPSE_DIR = PROJECT_DIR / "synapse"

SYNAPSE_BUILD_DIR = SYNAPSE_DIR / "build"
SYNAPSE_BIN_DIR = SYNAPSE_BUILD_DIR / "bin"

# Read by the NFs' symbex Makefile target; exported by paths.sh.
SYMBEX_ENV_VARS = ["KLEE_INCLUDE", "KLEE_BUILD_PATH", "RTE_SDK"]


@dataclass
class NF:
    name: str
    bdd: str

    def get_dir(self) -> Path:
        return NFS_DIR / self.name

    def get_call_paths_dir(self) -> Path:
        return self.get_dir() / "klee-last"

    def get_bdd(self) -> Path:
        return BDD_DIR / self.bdd

    def get_dot(self) -> Path:
        return self.get_bdd().with_suffix(".dot")


NFs = {
    "echo": NF("echo", "echo.bdd"),
    "fwd": NF("fwd", "fwd.bdd"),
    "fw": NF("fw", "fw.bdd"),
    "nat": NF("nat", "nat.bdd"),
    "kvs": NF("kvs", "kvs.bdd"),
    "psd": NF("psd", "psd.bdd"),
    "cl": NF("cl", "cl.bdd"),
    "pol": NF("pol", "pol.bdd"),
    "hhh": NF("hhh", "hhh.bdd"),
    "hyperloglog": NF("hyperloglog", "hyperloglog.bdd"),
    "smartcookie": NF("smartcookie", "smartcookie.bdd"),
    "meta4": NF("meta4", "meta4.bdd"),
}


def panic(msg: str):
    rich.print(f"ERROR: {msg}")
    exit(1)


def build_synapse(
    debug: bool,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    cmd = "./build-debug.sh" if debug else "./build-release.sh"

    files_consumed = []
    files_produced = [
        SYNAPSE_BIN_DIR / "call-paths-to-bdd",
        SYNAPSE_BIN_DIR / "bdd-inspector",
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


def run_symbex(
    nf: NF,
    force: bool,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    files_consumed = []
    files_produced = [nf.get_call_paths_dir()]

    # klee-last survives from earlier runs, so it cannot tell whether symbex is due: the BDD it
    # feeds can.
    up_to_date = nf.get_bdd().exists() and not force

    return Task(
        f"symbex_{nf.name}",
        "make symbex",
        cwd=nf.get_dir(),
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution or up_to_date,
        ignore_skip_if_already_produced=True,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def call_paths_to_bdd(
    nf: NF,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    files_consumed = [SYNAPSE_BIN_DIR / "call-paths-to-bdd", nf.get_call_paths_dir()]
    files_produced = [nf.get_bdd()]

    cmd = f"{SYNAPSE_BIN_DIR / 'call-paths-to-bdd'}"
    cmd += f" --out {nf.get_bdd()}"
    cmd += f" {nf.get_call_paths_dir()}/*.call_path"

    return Task(
        f"call_paths_to_bdd_{nf.name}",
        cmd,
        shell=True,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def visualize_bdd(
    nf: NF,
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    files_consumed = [SYNAPSE_BIN_DIR / "bdd-visualizer", nf.get_bdd()]
    files_produced = [nf.get_dot()]

    cmd = f"{SYNAPSE_BIN_DIR / 'bdd-visualizer'}"
    cmd += f" --in {nf.get_bdd()}"
    cmd += f" --out {nf.get_dot()}"

    return Task(
        f"visualize_bdd_{nf.name}",
        cmd,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def inspect_bdd(
    nf: NF,
    next: list[Task],
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    files_consumed = [SYNAPSE_BIN_DIR / "bdd-inspector", nf.get_bdd()]
    files_produced = []

    cmd = f"{SYNAPSE_BIN_DIR / 'bdd-inspector'}"
    cmd += f" --in {nf.get_bdd()}"

    return Task(
        f"inspect_bdd_{nf.name}",
        cmd,
        next=next,
        files_consumed=files_consumed,
        files_produced=files_produced,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


if __name__ == "__main__":
    description = "BDD generation script. For each NF: symbolically execute its DPDK implementation, build the BDD from the resulting call paths, inspect it, and render it."
    description += f" NFs dir: {NFS_DIR}."
    description += f" BDD dir: {BDD_DIR}."

    parser = ArgumentParser(description=description)

    parser.add_argument("--nfs", type=str, choices=NFs.keys(), nargs="+", default=list(NFs.keys()), help="Target NFs (default: all)")
    parser.add_argument("--debug", action="store_true", default=False, help="Build synapse in debug mode")

    parser.add_argument("--max-concurrent-tasks", type=int, default=-1, help="Maximum number of concurrent tasks to run. If <= 0, uses number of CPU cores.")
    parser.add_argument("--show-cmds-output", action="store_true", default=False, help="Show command output during execution")
    parser.add_argument("--show-cmds", action="store_true", default=False, help="Show requested commands during execution")
    parser.add_argument("--show-execution-plan", action="store_true", default=False, help="Show execution plan")
    parser.add_argument("--dry-run", action="store_true", default=False)
    parser.add_argument("--force", action="store_true", default=False, help="Force execution even if files are already produced")

    parser.add_argument("--silence", action="store_true", default=False, help="Silence all output except errors")

    args = parser.parse_args()

    missing_env_vars = [var for var in SYMBEX_ENV_VARS if var not in os.environ]
    if missing_env_vars:
        panic(f"{', '.join(missing_env_vars)} not set: source {PROJECT_DIR / 'paths.sh'} first")

    Path.mkdir(BDD_DIR, exist_ok=True)

    orchestrator = Orchestrator()

    orchestrator.add_task(
        build_synapse(
            debug=args.debug,
            skip_execution=args.dry_run,
            show_cmds_output=args.show_cmds_output,
            show_cmds=args.show_cmds,
            silence=args.silence,
        )
    )

    for nf_name in args.nfs:
        nf = NFs[nf_name]

        orchestrator.add_task(
            run_symbex(
                nf,
                force=args.force,
                skip_execution=args.dry_run,
                show_cmds_output=args.show_cmds_output,
                show_cmds=args.show_cmds,
                silence=args.silence,
            )
        )

        orchestrator.add_task(
            call_paths_to_bdd(
                nf,
                skip_execution=args.dry_run,
                show_cmds_output=args.show_cmds_output,
                show_cmds=args.show_cmds,
                silence=args.silence,
            )
        )

        # Rendered only once the BDD passes inspection.
        visualize_task = visualize_bdd(
            nf,
            skip_execution=args.dry_run,
            show_cmds_output=args.show_cmds_output,
            show_cmds=args.show_cmds,
            silence=args.silence,
        )

        orchestrator.add_task(
            inspect_bdd(
                nf,
                next=[visualize_task],
                skip_execution=args.dry_run,
                show_cmds_output=args.show_cmds_output,
                show_cmds=args.show_cmds,
                silence=args.silence,
            )
        )

        orchestrator.add_task(visualize_task)

    rich.print("============ Requested Configuration ============")
    rich.print(f"Target NFs:  {args.nfs}")
    rich.print(f"Total tasks: {orchestrator.size()}")
    rich.print("=================================================")

    if args.show_execution_plan:
        orchestrator.visualize()

    orchestrator.run(
        skip_if_already_produced=not args.force,
        max_concurrent_tasks=args.max_concurrent_tasks,
    )
