#!/usr/bin/env python3

import math

from experiments.hosts.controller import Controller
from experiments.hosts.switch import Switch
from experiments.hosts.pktgen import Pktgen

from experiments.experiment import Experiment

from pathlib import Path

from rich.console import Console
from rich.progress import Progress, TaskID

from typing import Optional

LO_PERF_CHURN_MBPS  = 10_000  # 10 Gbps
NUM_CHURN_STEPS     = 15
STARTING_CHURN_FPM  = 100

class ThroughputWithCPUCountersUnderChurn(Experiment):
    def __init__(
        self,
        
        # Experiment parameters
        name: str,
        save_name: Path,
        lo_churn_fpm: int,
        mid_churn_fpm: int,
        hi_churn_fpm: int,

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

        self.churns = self._get_churns(lo_churn_fpm, mid_churn_fpm, hi_churn_fpm)

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
        
        assert timeout_ms > 0

        self._sync()

    def _sync(self):
        header = f"#iteration, churn (fpm), #asic pkts, #cpu pkts, throughput (bps), throughput (pps)\n"

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
                    churn = float(cols[1])
                    self.experiment_tracker.add((i,churn,))
        else:
            with open(self.save_name, "w") as f:
                f.write(header)
    
    # Finds the churn at which performance starts dropping, and the one
    # that completely plummets it.
    def _find_churn_anchors(self, max_churn: int, step_progress: Progress, task_id: TaskID):
        lo_churn = STARTING_CHURN_FPM
        mid_churn = STARTING_CHURN_FPM
        hi_churn = max_churn
        multiplier = 10
        churn = lo_churn

        lo_churn_perf = None
        
        step_progress.update(task_id, description=f"Finding churn anchors...")
        self.log(f"************ CHURN ANCHOR SEARCH ************")

        while churn != max_churn:
            self.log(f"Finding churn anchors: {churn:,} fpm")

            throughput_bps, _, _ = self.find_stable_throughput(self.pktgen, churn, self.pkt_size)
            throughput_mbps = throughput_bps / 1e6

            step_progress.update(task_id, description=f"Finding churn anchors: {churn:,} fpm {int(throughput_mbps):,} Mbps")

            self.log(f"{churn:,} fpm => {int(throughput_mbps):,} Mbps")

            if lo_churn_perf is None:
                lo_churn_perf = throughput_mbps

            if throughput_mbps >= lo_churn_perf * 0.9:
                lo_churn = churn
            elif mid_churn == STARTING_CHURN_FPM:
                mid_churn = churn

            if throughput_mbps < LO_PERF_CHURN_MBPS:
                hi_churn = churn
                break

            churn = min(churn * multiplier, max_churn)
        
        if mid_churn == STARTING_CHURN_FPM:
            mid_churn = int((hi_churn + lo_churn) / 2)

        self.log(f"Churn anchors: lo={lo_churn:,} fpm mid={mid_churn:,} fpm hi={hi_churn:,} fpm")
        self.log(f"*********************************************")

        return lo_churn, mid_churn, hi_churn
    
    def _get_churns(self, lo_churn_fpm: int, mid_churn_fpm: int, hi_churn_fpm: int) -> list[int]:
        lo2mid_num_steps = math.ceil((NUM_CHURN_STEPS - 1) * (3/4))
        mid2hi_num_steps = max(0, NUM_CHURN_STEPS - lo2mid_num_steps - 1)

        lo2mid_churns_steps = int((mid_churn_fpm - lo_churn_fpm) / lo2mid_num_steps)
        mid2hi_churns_steps = int((hi_churn_fpm - mid_churn_fpm) / (mid2hi_num_steps - 1))

        self.churns = [ 0 ]
        self.churns = self.churns + [ int(lo_churn_fpm + i * lo2mid_churns_steps) for i in range(lo2mid_num_steps) ]
        self.churns = self.churns + [ int(mid_churn_fpm + i * mid2hi_churns_steps) for i in range(mid2hi_num_steps) ]

        self.log(f"Churns: {self.churns} fpm")
        return self.churns
    
    def _read_cpu_counters(
        self,
        churn: int,
        stable_rate_mbps: int,
    ) -> tuple[int, int]:
        self.log(f"Reading CPU counters (churn={churn:,} fpm rate={stable_rate_mbps:,} Mbps)")

        prev_counters = self.controller.get_cpu_counters()
        assert prev_counters

        self.log(f"Prev counters: in={prev_counters[0]:,} cpu={prev_counters[1]:,}")

        xput_bps, xput_pps, loss = self.test_throughput(
            stable_rate_mbps,
            self.pktgen,
            churn,
            self.pkt_size
        )

        self.log(f"Real throughput {int(xput_bps/1e6):,} Mbps {int(xput_pps/1e6):,} Mpps {100*loss:5.2f}% loss")

        post_counters = self.controller.get_cpu_counters()
        assert post_counters
        
        in_pkts = post_counters[0] - prev_counters[0]
        cpu_pkts = post_counters[1] - prev_counters[1]

        assert in_pkts > 0
        assert cpu_pkts >= 0
        
        self.log(f"Post counters: in={post_counters[0]:,} cpu={post_counters[1]:,}")
        self.log(f"Counters: in={in_pkts:,} cpu={cpu_pkts:,} (ratio={cpu_pkts/in_pkts})")

        return in_pkts, cpu_pkts

    def run(self, step_progress: Progress, current_iter: int) -> None:
        task_id = step_progress.add_task(self.name, total=NUM_CHURN_STEPS)

        # Check if we already have everything before running all the programs.
        completed = True
        for churn in self.churns:
            exp_key = (current_iter,churn,)
            if exp_key not in self.experiment_tracker:
                completed = False
                break
        if completed:
            return

        self.switch.install(
            self.p4_src_in_repo,
            self.p4_compile_time_vars,
        )

        self.pktgen.launch(
            self.nb_flows,
            self.pkt_size,
            self.timeout_ms * 1000,
            self.crc_unique_flows,
            self.crc_bits,
            mark_warmup_packets=True,
        )

        for i, churn in enumerate(self.churns):
            exp_key = (current_iter,churn,)

            description=f"it={current_iter} churn={churn:,} ({i+1}/{NUM_CHURN_STEPS})"
            step_progress.update(task_id, description=description)

            if exp_key in self.experiment_tracker:
                self.console.log(f"[orange1]Skipping: {description}")
                step_progress.update(task_id, description=description, advance=1)
                continue

            self.controller.launch(
                self.controller_src_in_repo,
                self.timeout_ms,
            )

            self.controller.wait_ready()
            max_churn = self.pktgen.wait_launch()

            if churn > max_churn:
                print(f"Error: requested churn is not allowed ({churn:,} > max churn {max_churn:,})")
                exit(1)

            throughput_bps, throughput_pps, requested_rate_mbps = self.find_stable_throughput(self.pktgen, churn, self.pkt_size)

            throughput_mbps = int(throughput_bps / 1e6)
            throughput_mpps = int(throughput_pps / 1e6)

            self.log(f"Churn {churn:,} => {throughput_mbps:,} Mbps {throughput_mpps:,} Mpps")
            step_progress.update(task_id, description=f"{description} [reading CPU counters]")

            in_pkts, cpu_pkts = self._read_cpu_counters(churn, requested_rate_mbps)

            with open(self.save_name, "a") as f:
                f.write(f"{current_iter},{churn},{in_pkts},{cpu_pkts},{throughput_bps},{throughput_pps}\n")

            step_progress.update(task_id, advance=1)
            self.controller.stop()

        self.pktgen.close()
        step_progress.update(task_id, visible=False)
