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

SPAWN_COLOR = "cyan"
DONE_COLOR = "green"
SKIP_COLOR = "yellow"
ERROR_COLOR = "red"
DEBUG_COLOR = "white"


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


class Task:
    def __init__(
        self,
        name: str,
        cmd: str,
        env_vars: dict[str, str] = {},
        cwd: Optional[Path] = None,
        prev: list["Task"] = [],
        next: list["Task"] = [],
        skip_execution: bool = False,
        show_cmds_output: bool = False,
        show_cmds: bool = False,
        silence: bool = False,
    ):
        self.name = name
        self.cmd = cmd
        self.env_vars = env_vars
        self.cwd = cwd
        self.prev = set(prev)
        self.next = set(next)
        self.skip_execution = skip_execution
        self.show_cmds_output = show_cmds_output
        self.show_cmds = show_cmds
        self.silence = silence

        self.done: bool = False

        for next_task in self.next:
            if not isinstance(next_task, Task):
                panic(f"Next task {next_task} is not a Task instance")
            next_task.prev.add(self)

        for prev_task in self.prev:
            if not isinstance(prev_task, Task):
                panic(f"Previous task {prev_task} is not a Task instance")
            prev_task.next.add(self)

    def __eq__(self, other: "Task"):
        if not isinstance(other, Task):
            return False
        return self.name == other.name and self.cmd == other.cmd and self.env_vars == other.env_vars and self.cwd == other.cwd

    def __hash__(self):
        return hash((self.name, self.cmd, frozenset(self.env_vars.items()), self.cwd))

    def __repr__(self):
        return f"Task(name={self.name})"

    def log(self, msg: str, color: Optional[str] = None):
        if self.silence and color != ERROR_COLOR:
            return

        prefix = ""
        suffix = ""
        if color:
            prefix = f"[{color}]"
            suffix = f"[/{color}]"

        rich.print(f"{prefix}\\[{self.name}] {msg}{suffix}", flush=True)

    def is_ready(self) -> bool:
        for prev_task in self.prev:
            if not prev_task.done:
                return False
        return True

    async def run(self):
        if self.done:
            self.log("already executed, skipping", color=SKIP_COLOR)
            return

        if self.show_cmds and not self.silence:
            flattened_env_vars = " ".join([f"{k}={v}" for k, v in self.env_vars.items()])
            print(f"[{self.name}] {flattened_env_vars} {self.cmd}")

        if self.skip_execution:
            self.log("skipping execution as requested", color=SKIP_COLOR)
            self.done = True
            self.elapsed_time = 0.0
            return

        self.log("spawning", color=SPAWN_COLOR)

        process = await asyncio.create_subprocess_exec(
            *self.cmd.split(" "),
            stdout=PIPE,
            stderr=STDOUT,
            bufsize=0,
            cwd=self.cwd,
            env={**self.env_vars, **dict(list(os.environ.items()))},
        )

        start_time = time.perf_counter()

        assert process, "Process creation failed"

        stdout_data, _ = await process.communicate()
        stdout = stdout_data.decode().strip() if stdout_data else ""

        if process.returncode != 0 or self.show_cmds_output:
            if process.returncode != 0:
                self.log(f"command failed with return code {process.returncode}", color=ERROR_COLOR)
                self.log("command:", color=ERROR_COLOR)
                print(self.cmd)
                self.log("output:", color=ERROR_COLOR)
                print(stdout)
                raise RuntimeError()
            elif not self.silence:
                print(stdout)

        self.done = True

        elapsed_time = time.perf_counter() - start_time
        delta = timedelta(seconds=elapsed_time)
        self.log(f"done ({humanize.precisedelta(delta, minimum_unit='milliseconds')})", color=DONE_COLOR)

    def dump_execution_plan(self, indent: int = 0):
        rich.print("  " * indent + f"{self}")
        for next_task in self.next:
            next_task.dump_execution_plan(indent + 1)


class ExecutionGraph:
    def __init__(self, initial_tasks: list[Task] = []):
        self.initial_tasks = initial_tasks

    def size(self) -> int:
        all_tasks = set()

        next_tasks = set(self.initial_tasks)
        while next_tasks:
            task = next_tasks.pop()
            all_tasks.add(task)
            next_tasks.update(task.next)

        return len(all_tasks)

    async def worker(self, queue: asyncio.Queue):
        task: Optional[Task] = None

        while True:
            try:
                task = await queue.get()
                if task is None:
                    break

                assert isinstance(task, Task), f"Expected Task instance, got {type(task)}"
                await task.run()

                for next_task in task.next:
                    if next_task.is_ready():
                        queue.put_nowait(next_task)
            except Exception as e:
                if not task:
                    print(f"Exception caught while waiting for task: {e}", file=sys.stderr)
            finally:
                queue.task_done()

    async def _run(self):
        queue = asyncio.Queue()

        cpu_count = os.cpu_count() or 8

        for task in self.initial_tasks:
            queue.put_nowait(task)

        async_tasks = []
        for i in range(cpu_count):
            task = asyncio.create_task(self.worker(queue))
            async_tasks.append(task)

        await queue.join()

        for task in async_tasks:
            task.cancel()

        await asyncio.gather(*async_tasks, return_exceptions=True)

    def run(self):
        start = time.perf_counter()
        asyncio.run(self._run())
        end = time.perf_counter()

        delta = timedelta(seconds=end - start)
        rich.print(f"[{DONE_COLOR}]Execution time: {humanize.precisedelta(delta, minimum_unit='milliseconds')}[/{DONE_COLOR}]")

    def dump_execution_plan(self):
        for task in self.initial_tasks:
            task.dump_execution_plan()


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
    prev: list[Task] = [],
    next: list[Task] = [],
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    cmd = "./build.sh"

    return Task(
        "build_synapse",
        cmd,
        cwd=SYNAPSE_DIR,
        prev=prev,
        next=next,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def generate_pcap(
    nf: NF,
    total_packets: int,
    packet_size: int,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
    prev: list[Task] = [],
    next: list[Task] = [],
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
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
        prev=prev,
        next=next,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def generate_profiler(
    nf: NF,
    prev: list[Task] = [],
    next: list[Task] = [],
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    profiler_name = f"{nf.name}-profiler.cpp"

    synthesize_profiler_cmd = f"{SYNAPSE_BIN_DIR / 'bdd-synthesizer'}"
    synthesize_profiler_cmd += f" --target profiler"
    synthesize_profiler_cmd += f" --in {BDD_DIR / nf.bdd}"
    synthesize_profiler_cmd += f" --out {SYNTHESIZED_DIR / profiler_name}"

    return Task(
        f"generate_profiler_{nf.name}",
        synthesize_profiler_cmd,
        prev=prev,
        next=next,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def compile_profiler(
    nf: NF,
    prev: list[Task] = [],
    next: list[Task] = [],
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
):
    profiler_name = f"{nf.name}-profiler"
    compile_profiler_cmd = f"make -f {TOOLS_DIR / 'Makefile.dpdk'}"

    return Task(
        f"compile_profiler_{nf.name}",
        compile_profiler_cmd,
        env_vars={"NF": f"{SYNTHESIZED_DIR / profiler_name}.cpp"},
        prev=prev,
        next=next,
        skip_execution=skip_execution,
        show_cmds_output=show_cmds_output,
        show_cmds=show_cmds,
        silence=silence,
    )


def profile_nf_against_pcap(
    nf: NF,
    total_flows: int,
    zipf_param: float,
    churn_fpm: int,
    prev: list[Task] = [],
    next: list[Task] = [],
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    profiler_name = f"{nf.name}-profiler"
    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)

    report = f"{pcap_base_name}.json"

    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)
    profile_cmd = f"{SYNTHESIZED_DIR / 'build' / profiler_name}"
    profile_cmd += f" {PROFILE_DIR / report}"
    for warmup_dev in nf.clients:
        warmup_pcap = f"{pcap_base_name}-dev{warmup_dev}-warmup.pcap"
        profile_cmd += f" --warmup {warmup_dev}:{PCAP_DIR / warmup_pcap}"
    for dev in DEVICES:
        pcap = f"{pcap_base_name}-dev{dev}.pcap"
        profile_cmd += f" {dev}:{PCAP_DIR / pcap}"

    return Task(
        f"profile_nf_against_pcap_{nf.name}_f{total_flows}_c{churn_fpm}_zipf{zipf_param}",
        profile_cmd,
        prev=prev,
        next=next,
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
    prev: list[Task] = [],
    next: list[Task] = [],
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)

    report = f"{pcap_base_name}.json"
    dot = f"{pcap_base_name}.dot"

    profile_visualizer_cmd = f"{SYNAPSE_BIN_DIR / 'bdd-visualizer'}"
    profile_visualizer_cmd += f" --in {BDD_DIR / nf.bdd}"
    profile_visualizer_cmd += f" --profile {PROFILE_DIR / report}"
    profile_visualizer_cmd += f" --out {PROFILE_DIR / dot}"

    return Task(
        f"generate_profile_visualizer_{nf.name}_f{total_flows}_c{churn_fpm}_zipf{zipf_param}",
        profile_visualizer_cmd,
        prev=prev,
        next=next,
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
    prev: list[Task] = [],
    next: list[Task] = [],
    skip_execution: bool = False,
    show_cmds_output: bool = False,
    show_cmds: bool = False,
    silence: bool = False,
) -> Task:
    pcap_base_name = get_pcap_base_name(nf, total_flows, zipf_param, churn_fpm)

    report = f"{pcap_base_name}.json"
    stats = f"{pcap_base_name}.txt"

    profile_stats_cmd = f"{TOOLS_DIR / 'profile_stats.py'}"
    profile_stats_cmd += f" {PROFILE_DIR / report}"
    profile_stats_cmd += f" --out {PROFILE_DIR / stats}"

    return Task(
        f"generate_profile_stats_{nf.name}_f{total_flows}_c{churn_fpm}_zipf{zipf_param}",
        profile_stats_cmd,
        prev=prev,
        next=next,
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

    parser.add_argument("--nfs", type=str, choices=NFs.keys(), nargs="+", default=DEFAULT_NFS, help="Target NFs to profile (BDD)")
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

    build_synapse_task = build_synapse(
        skip_execution=args.dry_run,
        show_cmds_output=args.show_cmds_output,
        show_cmds=args.show_cmds,
        silence=args.silence,
    )

    execution_graph = ExecutionGraph([build_synapse_task])

    for nf_name in args.nfs:
        nf = NFs[nf_name]
        bdd = BDD_DIR / nf.bdd
        assert_bdd(bdd)

        profiler_generator_task = generate_profiler(
            nf,
            prev=[build_synapse_task],
            skip_execution=args.dry_run or args.skip_profiler_generation,
            show_cmds_output=args.show_cmds_output,
            show_cmds=args.show_cmds,
            silence=args.silence,
        )

        profiler_compilation_task = compile_profiler(
            nf,
            prev=[profiler_generator_task],
            skip_execution=args.dry_run,
            show_cmds_output=args.show_cmds_output,
            show_cmds=args.show_cmds,
            silence=args.silence,
        )

        for total_packets, packet_size, total_flows, zipf_param, churn in combinations:
            pcap_generation_task = generate_pcap(
                nf,
                total_packets,
                packet_size,
                total_flows,
                zipf_param,
                churn,
                prev=[build_synapse_task],
                skip_execution=args.dry_run or args.skip_pcap_generation,
                show_cmds_output=args.show_cmds_output,
                show_cmds=args.show_cmds,
                silence=args.silence,
            )

            profile_nf_against_pcap_task = profile_nf_against_pcap(
                nf,
                total_flows,
                zipf_param,
                churn,
                prev=[profiler_compilation_task, pcap_generation_task],
                skip_execution=args.dry_run,
                show_cmds_output=args.dry_run or args.show_cmds_output,
                show_cmds=args.show_cmds,
                silence=args.silence,
            )

            generate_profile_visualizer_task = generate_profile_visualizer(
                nf,
                total_flows,
                zipf_param,
                churn,
                prev=[profile_nf_against_pcap_task],
                skip_execution=args.dry_run,
                show_cmds_output=args.dry_run or args.show_cmds_output,
                show_cmds=args.show_cmds,
                silence=args.silence,
            )

            generate_profile_stats_task = generate_profile_stats(
                nf,
                total_flows,
                zipf_param,
                churn,
                prev=[profile_nf_against_pcap_task],
                skip_execution=args.dry_run,
                show_cmds_output=args.dry_run or args.show_cmds_output,
                show_cmds=args.show_cmds,
                silence=args.silence,
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
    rich.print(f"Total tasks:              {execution_graph.size()}")
    rich.print("=================================================")

    if args.show_execution_plan:
        execution_graph.dump_execution_plan()

    execution_graph.run()
