#!/usr/bin/env python3

import math

from pathlib import Path
from collections import defaultdict

from rich.console import Console
from rich.progress import Progress

from util.throughput import Throughput
from util.hosts.controller import Controller
from util.hosts.switch import Switch
from util.hosts.pktgen import Pktgen

DEFAULT_MID_PERF_CHURN  = 50_000 # 50 Gbps
DEFAULT_LOW_PERF_CHURN  = 1_000  # 1 Gbps
DEFAULT_NUM_CHURN_STEPS = 10
STARTING_CHURN          = 1_000  # 1000 fpm

class Churn(Throughput):
    def __init__(
        self,
        name: str,
        iterations: int,
        save_name: Path,
        switch: Switch,
        controller: Controller,
        pktgen: Pktgen,
        nb_churn_steps: int = DEFAULT_NUM_CHURN_STEPS,
        console: Console = Console()
    ) -> None:
        super().__init__(
            name=name,
            iterations=iterations,
            save_name=save_name,
            switch=switch,
            controller=controller,
            pktgen=pktgen,
            console=console,
        )
    
        assert nb_churn_steps % 2 == 0
        self.nb_churn_steps = nb_churn_steps

    def _sync(self):
        header = f"iteration,churn (fpm),throughput (bps),throughput (pps)\n"

        self.experiment_tracker = defaultdict(int)
        self.churns = defaultdict(int)

        # If file exists, continue where we left off.
        if self.save_name.exists():
            with open(self.save_name) as f:
                read_header = f.readline()
                assert header == read_header
                for row in f.readlines():
                    cols = row.split(",")
                    it = int(cols[0])
                    churn = int(cols[1])
                    self.experiment_tracker[it] += 1
                    self.churns[it] = churn

        else:
            with open(self.save_name, "w") as f:
                f.write(header)
    
    # Finds the churn at which performance drops to half, and the one
    # that completely plummets it.
    def _find_churn_anchors(self, max_churn: int):
        churn = STARTING_CHURN
        rate  = DEFAULT_MID_PERF_CHURN

        mid_churn  = None
        high_churn = None

        while True:
            if self.pktgen.log_file:
                self.pktgen.log_file.write(
                    f"Trying to find half perf churn: {churn:,} fpm\n"
                )

            throughput_bps, _ = self._find_stable_throughput(
                churn=churn,
                steps=1,
                max_rate=rate,
            )

            # Throughput equals 0 if it wasn't stable.
            if throughput_bps == 0:
                if mid_churn is None:
                    mid_churn = churn
                    rate      = DEFAULT_LOW_PERF_CHURN
                else:
                    high_churn = churn
                    break

            churn = min(churn * 2, max_churn)

            if churn == max_churn:
                mid_churn  = churn
                high_churn = churn
                break
        
        self.pktgen.log_file.write(f"Churn anchors: mid={mid_churn:,} fpm high={high_churn:,} fpm\n")

        return mid_churn, high_churn

    def run(self, step_progress: Progress, current_iter: int) -> None:
        task_id = step_progress.add_task(self.name, total=self.nb_churn_steps)

        # Check if we already have everything before running all the programs.
        completed = True
        for i in range(self.nb_churn_steps):
            if self.experiment_tracker[i] <= current_iter:
                completed = False
                break
        if completed:
            return

        self.switch.install()
        
        self.controller.launch()
        self.pktgen.launch()

        max_churn = self.pktgen.wait_launch()
        self.controller.wait_ready()

        for i in range(self.nb_churn_steps):
            if self.experiment_tracker[i] > current_iter:
                self.console.log(f"[orange1]Skipping: iteration {i+1}")
                step_progress.update(task_id, advance=1)
                continue

            if len(self.churns) != self.nb_churn_steps:
                step_progress.update(task_id, description=f"Finding churn anchors...")

                mid_churn, high_churn = self._find_churn_anchors(max_churn)
                mid_steps             = math.floor(self.nb_churn_steps * 3/4)
                high_steps            = self.nb_churn_steps - mid_steps
                mid_churns_steps      = int(mid_churn / mid_steps)
                high_churns_steps     = int((high_churn - mid_churn) / high_steps)
                mid_churns            = [ int(i * mid_churns_steps) for i in range(mid_steps) ]
                high_churns           = [ int(mid_churn + i * high_churns_steps) for i in range(1, high_steps + 1) ]
                self.churns           = mid_churns + high_churns

            churn = self.churns[i]

            if self.pktgen.log_file:
                self.pktgen.log_file.write(f"Trying churn {churn:,}\n")

            step_progress.update(
                task_id,
                description=f"[{i+1:2d}/{len(self.churns):2d}] {churn:,} fpm"
            )

            throughput_bps, throughput_pps = self._find_stable_throughput(
                churn=churn,
                steps=self.throughput_search_steps,
                max_rate=self.max_throughput_mbps,
            )

            with open(self.save_name, "a") as f:
                f.write(f"{i},{churn},{throughput_bps},{throughput_pps}\n")

            step_progress.update(task_id, advance=1)

        step_progress.update(task_id, visible=False)

        self.pktgen.close()
        self.controller.stop()
