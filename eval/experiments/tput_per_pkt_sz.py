#!/usr/bin/env python3

from experiments.tput import ThroughputHosts
from experiments.experiment import Experiment

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional


class ThroughputPerPacketSize(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        save_name: Path,
        pkt_sizes: list[int],
        # Hosts
        hosts: ThroughputHosts,
        # Switch
        p4_src_in_repo: str,
        # Controller
        controller_src_in_repo: str,
        controller_timeout_ms: int,
        # TG controller
        broadcast: list[int],
        symmetric: list[int],
        route: list[tuple[int, int]],
        # Pktgen
        nb_flows: int,
        experiment_log_file: Optional[str] = None,
        console: Console = Console(),
    ) -> None:
        super().__init__(name, experiment_log_file)

        # Experiment parameters
        self.save_name = save_name
        self.hosts = hosts
        self.pkt_sizes = pkt_sizes

        # Switch
        self.p4_src_in_repo = p4_src_in_repo

        # Controller
        self.controller_src_in_repo = controller_src_in_repo
        self.controller_timeout_ms = controller_timeout_ms

        # TG controller
        self.broadcast = broadcast
        self.symmetric = symmetric
        self.route = route

        # Pktgen
        self.nb_flows = nb_flows

        self.console = console

        self._sync()

    def _sync(self):
        header = f"#it, pkt size (bytes), requested (bps), pktgen tput (bps), pktgen tput (pps), DUT ingress (bps), DUT ingress (pps) DUT egress (bps) DUT egress (pps)\n"

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
                    self.experiment_tracker.add(
                        (
                            i,
                            pkt_size,
                        )
                    )
        else:
            with open(self.save_name, "w") as f:
                f.write(header)

    def run(
        self,
        step_progress: Progress,
        current_iter: int,
    ) -> None:
        task_id = step_progress.add_task(f"{self.name} (it={current_iter})", total=len(self.pkt_sizes))

        # Check if we already have everything before running all the programs.
        completed = True
        for pkt_size in self.pkt_sizes:
            exp_key = (
                current_iter,
                pkt_size,
            )
            if exp_key not in self.experiment_tracker:
                completed = False
                break
        if completed:
            return

        self.log("Installing Tofino TG")
        self.hosts.tg_switch.install()

        self.log("Installing NF")
        self.hosts.dut_switch.install(self.p4_src_in_repo)

        self.log("Launching Tofino TG")
        self.hosts.tg_switch.launch()

        self.log("Launching synapse controller")
        self.hosts.dut_controller.launch(self.controller_src_in_repo, self.controller_timeout_ms)

        self.log("Launching pktgen")
        self.hosts.pktgen.launch(
            nb_flows=int(self.nb_flows / len(self.broadcast)),
            pkt_size=self.pkt_sizes[0],
            exp_time_us=self.controller_timeout_ms * 1000,
        )

        self.log("Waiting for Tofino TG")
        self.hosts.tg_switch.wait_ready()

        self.log("Configuring Tofino TG")
        self.hosts.tg_controller.setup(
            broadcast=self.broadcast,
            symmetric=self.symmetric,
            route=self.route,
        )

        self.log("Waiting for pktgen")
        self.hosts.pktgen.wait_launch()

        self.log("Waiting for synapse controller")
        self.hosts.dut_controller.wait_ready()

        self.log("Waiting for TG ports")
        self.hosts.tg_controller.wait_for_ports()

        self.log("Starting experiment")

        for pkt_size in self.pkt_sizes:
            exp_key = (
                current_iter,
                pkt_size,
            )

            description = f"{self.name} (it={current_iter} pkt={pkt_size}B)"

            if exp_key in self.experiment_tracker:
                self.console.log(f"[orange1]Skipping: iteration {current_iter} pkt size {pkt_size}B")
                step_progress.update(task_id, description=description, advance=1)
                continue

            step_progress.update(task_id, description=description)

            self.log("Launching pktgen")
            self.hosts.pktgen.close()
            self.hosts.pktgen.launch(
                nb_flows=int(self.nb_flows / len(self.broadcast)),
                pkt_size=pkt_size,
                exp_time_us=self.controller_timeout_ms * 1000,
            )

            self.hosts.pktgen.wait_launch()

            report = self.find_stable_throughput(
                tg_controller=self.hosts.tg_controller,
                pktgen=self.hosts.pktgen,
                churn=0,
            )

            with open(self.save_name, "a") as f:
                f.write(f"{current_iter}")
                f.write(f",{pkt_size}")
                f.write(f",{report.requested_bps}")
                f.write(f",{report.pktgen_bps}")
                f.write(f",{report.pktgen_pps}")
                f.write(f",{report.dut_ingress_bps}")
                f.write(f",{report.dut_ingress_pps}")
                f.write(f",{report.dut_egress_bps}")
                f.write(f",{report.dut_egress_pps}")
                f.write(f"\n")

            step_progress.update(task_id, description=description, advance=1)

        self.hosts.pktgen.close()
        self.hosts.dut_controller.stop()

        step_progress.update(task_id, visible=False)
