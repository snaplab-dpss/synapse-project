#!/usr/bin/env python3

import math

from experiments.hosts.controller import Controller
from experiments.hosts.switch import Switch
from experiments.hosts.pktgen import Pktgen

from experiments.experiment import Experiment

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional

CPU_RATIOS: list[float] = [
    0,
    0.000001,
    0.00001,
    0.0001,
    0.001, 0.002, 0.003, 0.004, 0.005, 0.006, 0.007, 0.008, 0.009,
    0.01, 0.02, 0.03, 0.04, 0.05, 0.06, 0.07, 0.08, 0.09,
    0.1, 0.2, 0.3, 0.4, 0.5,
    1
]

class ThroughputPerCPURatio(Experiment):
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
        active_wait_iterations: int,

        # Pktgen
        nb_flows: int,
        pkt_size: int,
        crc_unique_flows: bool,
        crc_bits: int,
        
        # Extra
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
        self.active_wait_iterations = active_wait_iterations

        self.nb_flows = nb_flows
        self.pkt_size = pkt_size
        self.crc_unique_flows = crc_unique_flows
        self.crc_bits = crc_bits

        self.console = console
        
        self._sync()

    def _sync(self):
        header = f"#iteration, cpu ratio, #asic pkts, #cpu pkts, throughput (bps), throughput (pps)\n"

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
                    cpu_ratio = float(cols[1])
                    self.experiment_tracker.add((i,cpu_ratio,))
        else:
            with open(self.save_name, "w") as f:
                f.write(header)
    
    def _read_cpu_counters(
        self,
        cpu_ratio: float,
        stable_rate_mbps: int,
    ) -> tuple[int, int]:
        self.log(f"Reading CPU counters (ratio={cpu_ratio} rate={stable_rate_mbps:,} Mbps)")

        prev_counters = self.controller.get_cpu_counters()
        assert prev_counters

        self.log(f"Prev counters: in={prev_counters[0]:,} cpu={prev_counters[1]:,}")

        xput_bps, xput_pps, loss = self.test_throughput(
            stable_rate_mbps,
            self.pktgen,
            0,
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
        task_id = step_progress.add_task(self.name, total=len(CPU_RATIOS))

        # Check if we already have everything before running all the programs.
        completed = True
        for cpu_ratio in CPU_RATIOS:
            exp_key = (current_iter,cpu_ratio,)
            if exp_key not in self.experiment_tracker:
                completed = False
                break
        if completed:
            return

        self.switch.install(self.p4_src_in_repo)

        self.pktgen.launch(
            self.nb_flows,
            pkt_size=self.pkt_size,
            crc_unique_flows=self.crc_unique_flows,
            crc_bits=self.crc_bits
        )

        for cpu_ratio in CPU_RATIOS:
            exp_key = (current_iter,cpu_ratio,)

            description=f"{self.name} (it={current_iter} ratio={cpu_ratio})"

            if exp_key in self.experiment_tracker:
                self.console.log(f"[orange1]Skipping: iteration {current_iter} cpu ratio {cpu_ratio}")
                step_progress.update(task_id, description=description, advance=1)
                continue

            step_progress.update(task_id, description=description)

            self.controller.launch(
                self.controller_src_in_repo,
                extra_args=[
                    ("--ratio", cpu_ratio),
                    ("--iterations", int(self.active_wait_iterations)),
                ]
            )

            self.controller.wait_ready()
            self.pktgen.wait_launch()

            throughput_bps, throughput_pps, stable_thpt_mbps = self.find_stable_throughput(self.pktgen, 0, self.pkt_size)

            if stable_thpt_mbps == 0:
                self.log(f"Stable throughput is 0, skipping")
                self.controller.stop()
                step_progress.update(task_id, advance=1)
                continue

            step_progress.update(
                task_id,
                description=description + " [reading CPU counters]"
            )

            self.log(f"Ratio {cpu_ratio:,} => {int(throughput_bps/1e6):,} Mbps {int(throughput_pps/1e6):,} Mpps")
            in_pkts, cpu_pkts = self._read_cpu_counters(cpu_ratio, stable_thpt_mbps)

            with open(self.save_name, "a") as f:
                f.write(f"{current_iter},{cpu_ratio},{in_pkts},{cpu_pkts},{throughput_bps},{throughput_pps}\n")

            step_progress.update(task_id, advance=1)
            self.controller.stop()

        self.pktgen.close()
        step_progress.update(task_id, visible=False)
