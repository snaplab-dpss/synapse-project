#!/usr/bin/env python3

from experiments.experiment import Experiment

from hosts.synapse import SynapseController
from hosts.switch import Switch
from hosts.tofino_tg import TofinoTG, TofinoTGController
from hosts.pktgen import Pktgen

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import Optional


class ThroughputHosts:
    dut_switch: Switch
    controller: SynapseController
    tg_switch: TofinoTG
    tg_controller: TofinoTGController
    pktgen: Pktgen

    def __init__(
        self,
        config: dict,
    ) -> None:
        self.dut_switch = Switch(
            hostname=config["hosts"]["switch_dut"],
            repo=config["repo"]["switch_dut"],
            sde=config["devices"]["switch_dut"]["sde"],
            tofino_version=config["devices"]["switch_dut"]["tofino_version"],
            log_file=config["logs"]["switch_dut"],
        )

        self.controller = SynapseController(
            hostname=config["hosts"]["switch_dut"],
            repo=config["repo"]["switch_dut"],
            sde=config["devices"]["switch_dut"]["sde"],
            ports=config["devices"]["switch_dut"]["ports"],
            tofino_version=config["devices"]["switch_dut"]["tofino_version"],
            log_file=config["logs"]["controller_dut"],
        )

        self.tg_switch = TofinoTG(
            hostname=config["hosts"]["switch_tg"],
            repo=config["repo"]["switch_tg"],
            sde=config["devices"]["switch_tg"]["sde"],
            tofino_version=config["devices"]["switch_tg"]["tofino_version"],
            log_file=config["logs"]["switch_tg"],
        )

        self.tg_controller = TofinoTGController(
            hostname=config["hosts"]["switch_tg"],
            repo=config["repo"]["switch_tg"],
            sde=config["devices"]["switch_tg"]["sde"],
            log_file=config["logs"]["controller_tg"],
        )

        self.pktgen = Pktgen(
            hostname=config["hosts"]["pktgen"],
            repo=config["repo"]["pktgen"],
            rx_pcie_dev=config["devices"]["pktgen"]["rx_dev"],
            tx_pcie_dev=config["devices"]["pktgen"]["tx_dev"],
            nb_tx_cores=config["devices"]["pktgen"]["nb_tx_cores"],
            log_file=config["logs"]["pktgen"],
        )

    def terminate(self):
        self.tg_switch.kill_switchd()


class Throughput(Experiment):
    def __init__(
        self,
        # Experiment parameters
        name: str,
        save_name: Path,
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
        pkt_size: int,
        churn: int,
        experiment_log_file: Optional[str] = None,
        console: Console = Console(),
    ) -> None:
        super().__init__(name, experiment_log_file)

        # Experiment parameters
        self.save_name = save_name
        self.hosts = hosts

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
        self.pkt_size = pkt_size
        self.churn = churn

        self.console = console

        self._sync()

    def _sync(self):
        header = f"#it, pktgen tput (bps), pktgen tput (pps), tput (bps), tput (pps)\n"

        self.experiment_tracker = set()
        self.save_name.parent.mkdir(parents=True, exist_ok=True)

        # If file exists, continue where we left off.
        if self.save_name.exists():
            with open(self.save_name) as f:
                read_header = f.readline()
                assert header == read_header

                for row in f.readlines():
                    cols = row.split(",")
                    it = int(cols[0])
                    self.experiment_tracker.add(it)
        else:
            with open(self.save_name, "w") as f:
                f.write(header)

    def run(
        self,
        step_progress: Progress,
        current_iter: int,
    ) -> None:
        task_id = step_progress.add_task(self.name, total=1)

        if current_iter in self.experiment_tracker:
            self.console.log(f"[orange1]Skipping: {current_iter}")
            step_progress.update(task_id, advance=1)
            return

        self.log("Installing Tofino TG")
        self.hosts.tg_switch.install()

        self.log("Installing NF")
        self.hosts.dut_switch.install(self.p4_src_in_repo)

        self.log("Launching Tofino TG")
        self.hosts.tg_switch.launch()

        self.log("Launching synapse controller")
        self.hosts.controller.launch(self.controller_src_in_repo, self.controller_timeout_ms)

        self.log("Launching pktgen")
        self.hosts.pktgen.launch(
            nb_flows=self.nb_flows,
            pkt_size=self.pkt_size,
            exp_time_us=self.controller_timeout_ms * 1000,
        )

        self.log("Waiting for Tofino TG")
        self.hosts.tg_switch.wait_ready()

        self.log("Configuring Tofino TG")
        self.hosts.tg_controller.run(
            broadcast=self.broadcast,
            symmetric=self.symmetric,
            route=self.route,
        )

        self.log("Waiting for pktgen")
        self.hosts.pktgen.wait_launch()

        self.log("Waiting for synapse controller")
        self.hosts.controller.wait_ready()

        self.log("Starting experiment")

        step_progress.update(task_id, description=f"({current_iter})")

        report = self.find_stable_throughput(
            controller=self.hosts.controller,
            pktgen=self.hosts.pktgen,
            churn=self.churn,
            pkt_size=self.pkt_size,
            broadcast_ports=self.broadcast,
        )

        with open(self.save_name, "a") as f:
            f.write(f"{current_iter},{report.pktgen_bps},{report.pktgen_pps},{report.bps},{report.pps}\n")

        step_progress.update(task_id, advance=1)
        step_progress.update(task_id, visible=False)

        self.hosts.pktgen.close()
        self.hosts.controller.stop()
