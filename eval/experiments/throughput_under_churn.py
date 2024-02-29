#!/usr/bin/env python3

from experiments.hosts.controller import Controller
from experiments.hosts.switch import Switch
from experiments.hosts.pktgen import Pktgen

from experiments.experiment import Experiment, EXPERIMENT_ITERATIONS

from pathlib import Path

from rich.console import Console
from rich.progress import Progress, TaskID

from typing import Optional

LO_PERF_CHURN_MBPS  = 1_000  #  1 Gbps
NUM_CHURN_STEPS     = 10
STARTING_CHURN_FPM  = 100

class ThroughputUnderChurn(Experiment):
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
        p4_compile_time_vars: list[tuple[str,str]] = [],
        experiment_log_file: Optional[str] = None,
        console: Console = Console()
    ) -> None:
        super().__init__(name, experiment_log_file)

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

        self.p4_compile_time_vars = p4_compile_time_vars
        self.console = console
        
        self.churns = []
        assert timeout_ms > 0

        self._sync()

    def _sync(self):
        header = f"#iteration, churn (fpm), throughput (bps), throughput (pps)\n"

        self.experiment_tracker = { i: 0 for i in range(EXPERIMENT_ITERATIONS) }
        self.save_name.parent.mkdir(parents=True, exist_ok=True)

        # If file exists, continue where we left off.
        if self.save_name.exists():
            with open(self.save_name) as f:
                read_header = f.readline()
                assert header == read_header
                for row in f.readlines():
                    cols = row.split(",")
                    i = int(cols[0])
                    if i in self.experiment_tracker:
                        self.experiment_tracker[i] += 1
                    else:
                        self.experiment_tracker[i] = 1
        else:
            with open(self.save_name, "w") as f:
                f.write(header)
    
    # Finds the churn at which performance starts dropping, and the one
    # that completely plummets it.
    def _find_churn_anchors(self, max_churn: int, step_progress: Progress, task_id: TaskID):
        lo_churn = STARTING_CHURN_FPM
        mid_churn = lo_churn
        hi_churn = max_churn
        multiplier = 10
        churn = lo_churn

        lo_churn_perf = None
        
        self.log(f"************ CHURN ANCHOR SEARCH ************")

        while churn != max_churn:
            self.log(f"Finding churn anchors: {churn:,} fpm")

            throughput_bps, _ = self.find_stable_throughput(self.pktgen, churn, self.pkt_size)
            throughput_mbps = throughput_bps / 1e6

            step_progress.update(task_id, description=f"Finding churn anchors: {churn:,} fpm {throughput_mbps:.2f}Mbps")

            self.log(f"{churn:,} fpm => {throughput_mbps:,} Mbps")

            if lo_churn_perf is None:
                lo_churn_perf = throughput_mbps

            if throughput_mbps >= lo_churn_perf * 0.9:
                lo_churn = churn
            else:
                mid_churn = churn

            if throughput_mbps < LO_PERF_CHURN_MBPS:
                hi_churn = churn
                break

            churn = min(churn * multiplier, max_churn)

        self.log(f"Churn anchors: lo={lo_churn:,} fpm mid={mid_churn:,} fpm hi={hi_churn:,} fpm")
        self.log(f"*********************************************")

        return lo_churn, mid_churn, hi_churn
    
    def _get_churns(self, max_churn: int, step_progress: Progress, task_id: TaskID):
        if len(self.churns) == NUM_CHURN_STEPS:
            return self.churns
        
        step_progress.update(task_id, description=f"Finding churn anchors...")

        lo_churn, mid_churn, high_churn = self._find_churn_anchors(max_churn, step_progress, task_id)

        lo2mid_num_steps = int((NUM_CHURN_STEPS - 1) / 2)
        mid2hi_num_steps = max(0, NUM_CHURN_STEPS - lo2mid_num_steps - 1)

        lo2mid_churns_steps = int((mid_churn - lo_churn) / lo2mid_num_steps)
        mid2hi_churns_steps = int((high_churn - mid_churn) / (mid2hi_num_steps - 1))

        self.churns = [ 0 ]
        self.churns += [ int(lo_churn + i * lo2mid_churns_steps) for i in range(lo2mid_num_steps) ]
        self.churns += [ int(mid_churn + i * mid2hi_churns_steps) for i in range(mid2hi_num_steps) ]

        self.log(f"Churns: {self.churns} fpm")

        return self.churns

    def run(self, step_progress: Progress, current_iter: int) -> None:
        task_id = step_progress.add_task(self.name, total=NUM_CHURN_STEPS)

        # Check if we already have everything before running all the programs.
        if self.experiment_tracker[current_iter] >= NUM_CHURN_STEPS:
            return

        self.switch.install(
            self.p4_src_in_repo,
            self.p4_compile_time_vars,
        )

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
            if self.experiment_tracker[current_iter] > i:
                self.console.log(f"[orange1]Skipping: iteration {i}")
                step_progress.update(task_id, advance=1)
                continue

            churns = self._get_churns(max_churn, step_progress, task_id)
            churn = churns[i]

            step_progress.update(
                task_id,
                description=f"[{i+1:2d}/{len(churns):2d}] {churn:,} fpm"
            )

            throughput_bps, throughput_pps = self.find_stable_throughput(self.pktgen, churn, self.pkt_size)
            self.log(f"Churn {churn:,} => {throughput_bps/1e6:.2f} Mbps {throughput_pps/1e6:.2f} Mpps")

            with open(self.save_name, "a") as f:
                f.write(f"{current_iter},{churn},{throughput_bps},{throughput_pps}\n")

            step_progress.update(task_id, advance=1)

        self.pktgen.close()
        self.controller.stop()

        step_progress.update(task_id, visible=False)

