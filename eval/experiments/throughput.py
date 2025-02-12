#!/usr/bin/env python3

from experiments.experiment import Experiment

from experiments.hosts.synapse import SynapseController
from experiments.hosts.switch import Switch
from experiments.hosts.tofino_tg import TofinoTG, TofinoTGController
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
        dut_switch: Switch,
        controller: SynapseController,
        tg_switch: TofinoTG,
        tg_controller: TofinoTGController,
        pktgen: Pktgen,

        # Switch
        p4_src_in_repo: str,

        # Controller
        controller_src_in_repo: str,
        timeout_ms: int,

        # TG controller
        broadcast: list[int],
        symmetric: list[int],
        route: list[tuple[int,int]],

        # Pktgen
        nb_flows: int,
        pkt_size: int,
        churn: int,

        experiment_log_file: Optional[str] = None,
        console: Console = Console()
    ) -> None:
        super().__init__(name, experiment_log_file)
        
        # Experiment parameters
        self.save_name = save_name

        # Hosts
        self.dut_switch = dut_switch
        self.controller = controller
        self.tg_switch = tg_switch
        self.tg_controller = tg_controller
        self.pktgen = pktgen

        # Switch
        self.p4_src_in_repo = p4_src_in_repo
        
        # Controller
        self.controller_src_in_repo = controller_src_in_repo
        self.timeout_ms = timeout_ms

        # TG controller
        self.broadcast = broadcast
        self.symmetric = symmetric
        self.route = route

        # Pktgen
        self.nb_flows = nb_flows
        self.pkt_size = pkt_size
        self.churn = churn

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
    
        self.log("Installing Tofino TG")
        self.tg_switch.install()

        self.log("Installing NF")
        self.dut_switch.install(self.p4_src_in_repo)

        self.log("Launching Tofino TG")
        self.tg_switch.launch()
        
        self.log("Launching synapse controller")
        self.controller.launch(
            self.controller_src_in_repo,
            self.timeout_ms
        )

        self.log("Launching pktgen")
        self.pktgen.launch(
            nb_flows=self.nb_flows,
            pkt_size=self.pkt_size,
            exp_time_us=self.timeout_ms * 1000,
        )

        self.log("Waiting for Tofino TG")
        self.tg_switch.wait_ready()

        self.log("Configuring Tofino TG")
        self.tg_controller.run(
            broadcast=self.broadcast,
            symmetric=self.symmetric,
            route=self.route,
        )

        self.log("Waiting for pktgen")
        self.pktgen.wait_launch()

        self.log("Waiting for synapse controller")
        self.controller.wait_ready()

        self.log("Starting experiment")

        step_progress.update(task_id, description=f"({current_iter})")

        throughput_bps, throughput_pps, _ = self.find_stable_throughput(
            self.controller,
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
