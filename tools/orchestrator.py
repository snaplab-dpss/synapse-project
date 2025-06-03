#!/usr/bin/env python3

import os
import sys
import time
import humanize
import asyncio
import rich
import graphviz
import signal

from asyncio.subprocess import PIPE, STDOUT
from datetime import timedelta
from pathlib import Path
from typing import Optional

SPAWN_COLOR = "cyan"
DONE_COLOR = "green"
SKIP_COLOR = "yellow"
ERROR_COLOR = "red"
DEBUG_COLOR = "white"
ABORT_COLOR = "magenta"


class Task:
    def __init__(
        self,
        name: str,
        cmd: str,
        env_vars: dict[str, str] = {},
        cwd: Optional[Path] = None,
        # Relations with other tasks
        next: list["Task"] = [],
        # File dependencies
        files_consumed: list[Path] | list[str] = [],
        files_produced: list[Path] | list[str] = [],
        # Metadata
        skip_execution: bool = False,
        show_cmds_output: bool = False,
        show_cmds: bool = False,
        silence: bool = False,
    ):
        self.id = "_".join(name.split(" "))
        self.name = name
        self.cmd = cmd
        self.env_vars = env_vars
        self.cwd = cwd

        self.prev = set()
        self.next = set(next)

        self.files_consumed = set([Path(f) for f in files_consumed])
        self.files_produced = set([Path(f) for f in files_produced])

        self.skip_execution = skip_execution
        self.show_cmds_output = show_cmds_output
        self.show_cmds = show_cmds
        self.silence = silence

        self.done = False

        for next_task in self.next:
            next_task.prev.add(self)

    def __eq__(self, other: "Task"):
        if not isinstance(other, Task):
            return False
        if self.id != other.id:
            return False
        if self.cmd != other.cmd:
            return False
        if self.cwd != other.cwd:
            return False
        if self.env_vars != other.env_vars:
            return False
        return True

    def __hash__(self):
        return hash((self.name, self.cmd, frozenset(self.env_vars.items()), self.cwd))

    def __repr__(self):
        return f"Task(id={self.id})"

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

    def _start(self):
        self.start_time = time.perf_counter()

    def _end(self):
        self.done = True
        self.end_time = time.perf_counter()
        self.elapsed_time = self.end_time - self.start_time

    async def _run(
        self,
        skip_if_already_produced: bool = True,
    ) -> bool:
        if self.done:
            self.log("already executed, skipping", color=SKIP_COLOR)
            return True

        if self.show_cmds and not self.silence:
            flattened_env_vars = " ".join([f"{k}={v}" for k, v in self.env_vars.items()])
            print(f"[{self.name}] {flattened_env_vars} {self.cmd}")

        if self.skip_execution:
            self.log("skipping execution", color=SKIP_COLOR)
            return True

        for file in self.files_consumed:
            if not file.exists():
                self.log(f"consumed file '{file}' does not exist, skipping execution", color=ERROR_COLOR)
                return True

        all_files_produced = len(self.files_produced) > 0 and all([file.exists() for file in self.files_produced])
        if all_files_produced and skip_if_already_produced:
            self.log("all product files already exist, skipping execution", color=SKIP_COLOR)
            return True

        self.log("spawning", color=SPAWN_COLOR)

        try:
            process = await asyncio.create_subprocess_exec(
                *self.cmd.split(" "),
                stdout=PIPE,
                stderr=STDOUT,
                bufsize=0,
                cwd=self.cwd,
                env={**self.env_vars, **dict(list(os.environ.items()))},
            )

            assert process, "Process creation failed"

            stdout_data, _ = await process.communicate()
            stdout = stdout_data.decode().strip() if stdout_data else ""

            assert process.returncode is not None, "Process return code is None"

            if process.returncode < 0:
                self.log(f"terminated by signal {-process.returncode}", color=ABORT_COLOR)
                return False

            if process.returncode != 0 or self.show_cmds_output:
                if process.returncode != 0:
                    self.log(f"failed with return code {process.returncode}", color=ERROR_COLOR)
                    self.log("command:", color=ERROR_COLOR)
                    print(self.cmd)
                    self.log("output:", color=ERROR_COLOR)
                    print(stdout)
                    return False
                elif not self.silence:
                    print(stdout)
        except asyncio.CancelledError:
            self.log("task was cancelled", color=ERROR_COLOR)
            return False
        except Exception as e:
            self.log(f"error while executing command: {e}", color=ERROR_COLOR)
            return False

        return True

    async def run(
        self,
        skip_if_already_produced: bool = True,
    ) -> bool:
        self._start()
        success = await self._run(skip_if_already_produced)
        self._end()

        if success:
            missing_product_files = [f for f in self.files_produced if not f.exists()]
            if missing_product_files:
                self.log(f"some produced files are missing: {', '.join(str(f) for f in missing_product_files)}", color=ERROR_COLOR)
                success = False
            else:
                delta = timedelta(seconds=self.elapsed_time)
                self.log(f"done ({humanize.precisedelta(delta, minimum_unit='milliseconds')})", color=DONE_COLOR)

        return success

    def dump_execution_plan(self, indent: int = 0):
        rich.print("  " * indent + f"{self}")
        for next_task in self.next:
            next_task.dump_execution_plan(indent + 1)


class Orchestrator:
    def __init__(self, tasks: list[Task] = []):
        self.initial_tasks: set[Task] = set()
        self.files_to_producers: dict[Path, Task] = {}

        pending_tasks = set(tasks)
        while pending_tasks:
            task = pending_tasks.pop()
            self.add_task(task)
            pending_tasks.update(task.next)

    def add_task(self, task: Task):
        for file in task.files_produced:
            if file in self.files_to_producers:
                assert self.files_to_producers[file] == task, f"File {file} is already produced by another task: {self.files_to_producers[file].name}"
            self.files_to_producers[file] = task

        for file in task.files_consumed:
            if file in self.files_to_producers:
                task.prev.add(self.files_to_producers[file])
                self.files_to_producers[file].next.add(task)

        if len(task.prev) == 0:
            self.initial_tasks.add(task)

    def get_all_tasks(self) -> set[Task]:
        all_tasks = set()

        next_tasks = set(self.initial_tasks)
        while next_tasks:
            task = next_tasks.pop()
            all_tasks.add(task)
            next_tasks.update(task.next)

        return all_tasks

    def size(self) -> int:
        return len(self.get_all_tasks())

    def visualize(self, dot_file: Optional[str | Path] = None):
        dot = graphviz.Digraph(comment="Execution Plan", format="pdf")
        initial_graph = graphviz.Digraph(name="cluster_0", comment="Initial Tasks")
        final_graph = graphviz.Digraph(name="cluster_1", comment="Final Tasks")

        dot.attr(kw="graph", rankdir="LR")
        dot.attr(kw="node", style="filled", shape="box")
        initial_graph.attr(label="Initial Tasks")
        final_graph.attr(label="Final Tasks")

        assert initial_graph, "Failed to create subgraph for initial tasks"

        all_tasks = self.get_all_tasks()

        for task in all_tasks:
            color = SPAWN_COLOR if not task.skip_execution else SKIP_COLOR

            if task in self.initial_tasks:
                initial_graph.node(task.id, label=task.name, fillcolor=color)
            elif len(task.next) == 0:
                final_graph.node(task.id, label=task.name, fillcolor=color)
            else:
                dot.node(task.id, label=task.name, fillcolor=color)

        dot.subgraph(initial_graph)
        dot.subgraph(final_graph)

        for task in all_tasks:
            for next_task in task.next:
                dot.edge(task.id, next_task.id)

        if not dot_file:
            dot_file = Path("/tmp") / f"execution_plan_{int(time.time())}.dot"
            print(f"Saving execution plan to {dot_file}")

        dot.render(dot_file)

    async def _worker(
        self,
        queue: asyncio.Queue,
        skip_if_already_produced: bool,
    ):
        task: Optional[Task] = None

        while True:
            try:
                task = await queue.get()
                if task is None:
                    break

                assert isinstance(task, Task), f"Expected Task instance, got {type(task)}"
                success = await task.run(skip_if_already_produced)

                if success:
                    for next_task in task.next:
                        if next_task.is_ready():
                            queue.put_nowait(next_task)
            except Exception as e:
                rich.print(f"[{ERROR_COLOR}]Error in worker: {e}[/{ERROR_COLOR}]")
                pass
            finally:
                queue.task_done()

    async def _run(
        self,
        skip_if_already_produced: bool,
    ):
        queue = asyncio.Queue()
        loop = asyncio.get_running_loop()
        shutdown_event = asyncio.Event()

        cpu_count = os.cpu_count() or 8

        def handle_sigint():
            shutdown_event.set()

        if hasattr(signal, "SIGINT"):
            loop.add_signal_handler(signal.SIGINT, handle_sigint)
        if hasattr(signal, "SIGTERM"):
            loop.add_signal_handler(signal.SIGTERM, handle_sigint)

        for task in self.initial_tasks:
            queue.put_nowait(task)

        async_tasks = []
        for i in range(cpu_count):
            task = asyncio.create_task(self._worker(queue, skip_if_already_produced))
            async_tasks.append(task)

        await queue.join()

        for task in async_tasks:
            task.cancel()

        await asyncio.gather(*async_tasks, return_exceptions=True)

    def run(
        self,
        skip_if_already_produced: bool = True,
    ):
        start = time.perf_counter()
        asyncio.run(self._run(skip_if_already_produced))
        end = time.perf_counter()

        delta = timedelta(seconds=end - start)
        rich.print(f"[{DONE_COLOR}]Execution time: {humanize.precisedelta(delta, minimum_unit='milliseconds')}[/{DONE_COLOR}]")

    def dump_execution_plan(self):
        for task in self.initial_tasks:
            task.dump_execution_plan()
