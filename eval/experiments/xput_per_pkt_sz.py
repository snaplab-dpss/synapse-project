#!/usr/bin/env python3

from experiments.hosts.controller import Controller
from experiments.hosts.switch import Switch
from experiments.hosts.pktgen import Pktgen

from experiments.experiment import Experiment

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional

PACKET_SIZES_BYTES = [ 64, 128, 256, 512, 1024, 1500 ]

class ThroughputPerPacketSize(Experiment):
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
        churn: int,
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
        self.timeout_ms = timeout_ms

        self.nb_flows = nb_flows
        self.churn = churn
        self.crc_unique_flows = crc_unique_flows
        self.crc_bits = crc_bits

        self.console = console

        self._sync()

    def _sync(self):
        header = f"#iteration, pkt size (bytes), throughput (bps), throughput (pps)\n"

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
                    pkt_size = int(cols[1])
                    self.experiment_tracker.add((i,pkt_size,))
        else:
            with open(self.save_name, "w") as f:
                f.write(header)
    
    def run(
        self,
        step_progress: Progress,
        current_iter: int,
    ) -> None:
        task_id = step_progress.add_task(f"{self.name} (it={current_iter})", total=len(PACKET_SIZES_BYTES))

        # Check if we already have everything before running all the programs.
        completed = True
        for pkt_size in PACKET_SIZES_BYTES:
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

        for pkt_size in PACKET_SIZES_BYTES:
            exp_key = (current_iter,pkt_size,)
            
            description=f"{self.name} (it={current_iter} pkt={pkt_size}B)"

            if exp_key in self.experiment_tracker:
                self.console.log(f"[orange1]Skipping: iteration {current_iter} pkt size {pkt_size}B")
                step_progress.update(task_id, description=description, advance=1)
                continue

            step_progress.update(task_id, description=description)

            self.pktgen.launch(
                self.nb_flows,
                pkt_size,
                self.timeout_ms * 1000,
                self.crc_unique_flows,
                self.crc_bits
            )

            self.pktgen.wait_launch()
            self.controller.wait_ready()

            throughput_bps, throughput_pps, _ = self.find_stable_throughput(
                self.pktgen,
                self.churn,
                pkt_size
            )

            with open(self.save_name, "a") as f:
                f.write(f"{current_iter},{pkt_size},{throughput_bps},{throughput_pps}\n")

            step_progress.update(task_id, description=description, advance=1)

            self.pktgen.close()
        
        self.controller.stop()
        
        step_progress.update(task_id, visible=False)
