#!/usr/bin/env python3

from experiments.hosts.controller import Controller
from experiments.hosts.switch import Switch
from experiments.hosts.pktgen import Pktgen

from experiments.experiment import Experiment

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

import math

MID_PERF_CHURN  = 50_000 # 50 Gbps
LOW_PERF_CHURN  = 1_000  # 1 Gbps
NUM_CHURN_STEPS = 10
STARTING_CHURN  = 1_000  # 1000 fpm

class Churn(Experiment):
    def __init__(
        self,
        
        # Experiment parameters
        name: str,
        save_name: Path,

        # Hosts
        switch: Switch,
        controller: Controller,
        pktgen: Pktgen,

        # Switch
        p4_src_in_repo: str,

        # Controller
        controller_src_in_repo: str,
        timeout_ms: int,

        # Pktgen
        nb_flows: int,
        pkt_size: int,
        crc_unique_flows: bool,
        crc_bits: int,
        
        # Extra
        console: Console = Console()
    ) -> None:
        super().__init__(name)

        self.save_name = save_name

        self.switch = switch
        self.controller = controller
        self.pktgen = pktgen

        self.p4_src_in_repo = p4_src_in_repo
        
        self.controller_src_in_repo = controller_src_in_repo
        self.timeout_ms = timeout_ms

        self.nb_flows = nb_flows
        self.pkt_size = pkt_size
        self.crc_unique_flows = crc_unique_flows
        self.crc_bits = crc_bits

        self.console = console

        assert timeout_ms > 0

    def _sync(self):
        header = f"#iteration, churn (fpm), throughput (bps), throughput (pps)\n"

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
                    churn_fpm = int(cols[1])
                    self.experiment_tracker.add((i,churn_fpm,))
        else:
            with open(self.save_name, "w") as f:
                f.write(header)
    
    # Finds the churn at which performance drops to half, and the one
    # that completely plummets it.
    def _find_churn_anchors(self, max_churn: int):
        churn = STARTING_CHURN
        rate  = MID_PERF_CHURN

        mid_churn  = None
        high_churn = None

        while True:
            self.pktgen.host.log(f"Trying to find half perf churn: {churn:,} fpm")

            throughput_bps, _ = self.find_stable_throughput(self.pktgen, churn, self.pkt_size)

            # Throughput equals 0 if it wasn't stable.
            if throughput_bps == 0:
                if mid_churn is None:
                    mid_churn = churn
                    rate      = LOW_PERF_CHURN
                else:
                    high_churn = churn
                    break

            churn = min(churn * 2, max_churn)

            if churn == max_churn:
                mid_churn  = churn
                high_churn = churn
                break
        
        self.pktgen.host.log(f"Churn anchors: mid={mid_churn:,} fpm high={high_churn:,} fpm")

        return mid_churn, high_churn

    def run(self, step_progress: Progress, current_iter: int) -> None:
        task_id = step_progress.add_task(self.name, total=NUM_CHURN_STEPS)

        # Check if we already have everything before running all the programs.
        completed = True
        for i in range(NUM_CHURN_STEPS):
            exp_key = (current_iter,pkt_size,)
            if exp_key not in self.experiment_tracker:
                completed = False
                break
        if completed:
            return

        self.switch.install(self.p4_src_in_repo)
        
        self.controller.launch(
            self.controller_src_in_repo,
            self.timeout_ms
        )

        self.pktgen.launch(
            self.nb_flows,
            self.pkt_size,
            self.timeout_ms * 1000,
            self.crc_unique_flows,
            self.crc_bits
        )

        max_churn = self.pktgen.wait_launch()
        self.controller.wait_ready()

        for i in range(NUM_CHURN_STEPS):
            if self.experiment_tracker[i] > current_iter:
                self.console.log(f"[orange1]Skipping: iteration {i+1}")
                step_progress.update(task_id, advance=1)
                continue

            if len(self.churns) != NUM_CHURN_STEPS:
                step_progress.update(task_id, description=f"Finding churn anchors...")

                mid_churn, high_churn = self._find_churn_anchors(max_churn)
                mid_steps             = math.floor(NUM_CHURN_STEPS * 3/4)
                high_steps            = NUM_CHURN_STEPS - mid_steps
                mid_churns_steps      = int(mid_churn / mid_steps)
                high_churns_steps     = int((high_churn - mid_churn) / high_steps)
                mid_churns            = [ int(i * mid_churns_steps) for i in range(mid_steps) ]
                high_churns           = [ int(mid_churn + i * high_churns_steps) for i in range(1, high_steps + 1) ]
                self.churns           = mid_churns + high_churns

            churn = self.churns[i]

            self.pktgen.host.log(f"Trying churn {churn:,}\n")

            step_progress.update(
                task_id,
                description=f"[{i+1:2d}/{len(self.churns):2d}] {churn:,} fpm"
            )

            throughput_bps, throughput_pps = self.find_stable_throughput(self.pktgen, churn, self.pkt_size)

            with open(self.save_name, "a") as f:
                f.write(f"{i},{churn},{throughput_bps},{throughput_pps}\n")

            step_progress.update(task_id, advance=1)

        step_progress.update(task_id, visible=False)

        self.pktgen.close()
        self.controller.stop()
