#!/usr/bin/env python3

from experiments.experiment import Experiment

from eval.experiments.hosts.synapse import Controller
from experiments.hosts.switch import Switch
from experiments.hosts.pktgen import Pktgen

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional

class Throughput(Experiment):
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
        switch_pktgen_in_port: int,
        switch_pktgen_out_port: int,
        timeout_ms: int,

        # Pktgen
        nb_flows: int,
        pkt_size: int,
        churn: int,
        crc_unique_flows: bool,
        crc_bits: int,

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
        self.switch_pktgen_in_port = switch_pktgen_in_port
        self.switch_pktgen_out_port = switch_pktgen_out_port
        self.timeout_ms = timeout_ms

        self.nb_flows = nb_flows
        self.pkt_size = pkt_size
        self.churn = churn
        self.crc_unique_flows = crc_unique_flows
        self.crc_bits = crc_bits

        self.console = console

        self._sync()

    def _sync(self):
        header = f"throughput (bps),throughput (pps)\n"

        self.experiment_tracker = 0
        self.save_name.parent.mkdir(parents=True, exist_ok=True)

        # If file exists, continue where we left off.
        if self.save_name.exists():
            with open(self.save_name) as f:
                read_header = f.readline()
                assert header == read_header
                for row in f.readlines():
                    # pps
                    end = row.rfind(",")
                    row = row[:end]

                    # bps
                    end = row.rfind(",")
                    row = row[:end]

                    self.experiment_tracker += 1
        else:
            with open(self.save_name, "w") as f:
                f.write(header)

    def run(
        self,
        step_progress: Progress,
        current_iter: int,
    ) -> None:
        task_id = step_progress.add_task(self.name, total=1)

        if self.experiment_tracker > current_iter:
            self.console.log(f"[orange1]Skipping: {current_iter}")
            step_progress.update(task_id, advance=1)
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

        self.pktgen.wait_launch()
        self.controller.wait_ready()

        step_progress.update(task_id, description=f"({current_iter})")

        throughput_bps, throughput_pps, _ = self.find_stable_throughput(
            self.pktgen,
            self.churn,
            self.pkt_size
        )

        with open(self.save_name, "a") as f:
            f.write(f"{throughput_bps},{throughput_pps}\n")

        step_progress.update(task_id, advance=1)
        step_progress.update(task_id, visible=False)

        self.pktgen.close()
        self.controller.stop()
