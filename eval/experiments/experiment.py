from time import sleep
from datetime import datetime
from typing import Optional
from pathlib import Path
from dataclasses import dataclass

from rich.console import Group
from rich.live import Live
from rich.progress import (
    BarColumn,
    Progress,
    TextColumn,
    TimeElapsedColumn,
    TimeRemainingColumn,
)

from hosts.tofino_tg import TofinoTGController
from hosts.pktgen import Pktgen

MIN_THROUGHPUT = 100  # 100 Mbps
MAX_THROUGHPUT = 100_000  # 100 Gbps
ITERATION_DURATION_SEC = 5
MAX_ACCEPTABLE_LOSS = 0.001  # 0.1%
PORT_SETUP_PRECISION = 0.1  # 10%
PORT_SETUP_TIME_SEC = 5
PORT_SETUP_RATE = 1  # 1 Mbps
WARMUP_TIME_SEC = 5
WARMUP_RATE = 1_000  # 1 Gbps
REST_TIME_SEC = 10
BOGUS_RETRIES = 1
MAX_WARMUP_RETRIES = 10

DEFAULT_THROUGHPUT_SEARCH_STEPS = 10
DEFAULT_EXPERIMENT_ITERATIONS = 5


@dataclass
class ThroughputReport:
    requested_bps: int
    pktgen_bps: int
    pktgen_pps: int
    dut_ingress_bps: int
    dut_ingress_pps: int
    dut_egress_bps: int
    dut_egress_pps: int
    loss: float

    def __str__(self):
        s = ""
        s += f"Requested:   {self.requested_bps/1e9:12.5f} Gbps\n"
        s += f"Pktgen:      {self.pktgen_bps/1e9:12.5f} Gbps {self.pktgen_pps/1e6:12.5f} Mpps\n"
        s += f"DUT ingress: {self.dut_ingress_bps/1e9:12.5f} Gbps {self.dut_ingress_pps/1e6:12.5f} Mpps\n"
        s += f"DUT egress:  {self.dut_egress_bps/1e9:12.5f} Gbps {self.dut_egress_pps/1e6:12.5f} Mpps\n"
        s += f"Loss:        {self.loss*100:12.5f}%"
        return s


class Experiment:
    def __init__(
        self,
        name: str,
        log_file: Optional[str] = None,
        iterations: int = DEFAULT_EXPERIMENT_ITERATIONS,
    ) -> None:
        self.name = name
        self.iterations = iterations

        if log_file:
            out_file_path = Path(log_file)

            if out_file_path.exists():
                assert out_file_path.is_file()
                self.log_file = open(log_file, "a")
            else:
                out_file_path.parents[0].mkdir(parents=True, exist_ok=True)
                self.log_file = open(log_file, "w")
        else:
            self.log_file = None

        self.log()
        self.log(f"=================== {name} ===================")

    def log(self, msg=""):
        now = datetime.now()
        ts = now.strftime("%Y-%m-%d %H:%M:%S")
        if self.log_file:
            print(f"[{ts}][{self.name}] {msg}", file=self.log_file, flush=True)
        else:
            print(f"[{ts}][{self.name}] {msg}", flush=True)

    def run(self, step_progress: Progress, current_iter: int) -> None:
        raise NotImplementedError

    def run_many(self, progress: Progress, step_progress: Progress) -> None:
        task_id = progress.add_task("", total=self.iterations, name=self.name)
        for iter in range(self.iterations):
            self.run(step_progress, iter)
            progress.update(task_id, advance=1)

        progress.update(task_id, description="[bold green] done!")

    def warmup_ports(
        self,
        tg_controller: TofinoTGController,
        pktgen: Pktgen,
    ) -> None:
        tg_controller.set_acceleration(1)
        pktgen.set_rate(PORT_SETUP_RATE)

        tg_controller.wait_for_ports()

        ports_are_ready = False
        while not ports_are_ready:
            ports_are_ready = True

            meta_port_stats_old = tg_controller.get_port_stats_from_meta_table()
            pktgen.start()
            sleep(PORT_SETUP_TIME_SEC)
            pktgen.stop()
            meta_port_stats_new = tg_controller.get_port_stats_from_meta_table()

            self.log()
            self.log(f"Meta stats old {meta_port_stats_old}")
            self.log(f"Meta stats new {meta_port_stats_new}")

            tx_reference = None
            for port in tg_controller.broadcast_ports:
                tx = max(0, meta_port_stats_new[port].FramesTransmittedOK - meta_port_stats_old[port].FramesTransmittedOK)
                self.log(f"Port {port} TX {tx}")

                if tx_reference is None and tx != 0:
                    tx_reference = tx

                if tx_reference is not None:
                    rel_diff = abs(tx - tx_reference) / tx_reference
                    if rel_diff > PORT_SETUP_PRECISION:
                        ports_are_ready = False
                        self.log(f"Port {port} is not ready yet ({rel_diff} > {PORT_SETUP_PRECISION}). Retrying...")
                        sleep(1)
                        break

        self.log("Ports are ready now.")
        self.log()

        tg_controller.reset_to_default_acceleration()

    @staticmethod
    def _mac_pkt_deltas(mac_old, mac_new, ports) -> tuple[int, int]:
        """Sum (rx, tx) frame deltas over `ports` from two MAC ($PORT_STAT) snapshots.

        We use the MAC FramesReceivedOK/FramesTransmittedOK counters (read
        atomically per port) rather than the traffic-generator's P4 in/out
        counters: the P4 counters are two separate tables read in two separate,
        non-atomic entry_get calls whose from_hw read under-counts, so their ratio
        is dominated by measurement noise (it can even go negative). The MAC
        counters give the true, balanced rx/tx.
        """
        nb_rx = 0
        nb_tx = 0
        for port, stats in mac_new.items():
            if (port in ports) and (port in mac_old):
                nb_rx += stats.FramesReceivedOK - mac_old[port].FramesReceivedOK
                nb_tx += stats.FramesTransmittedOK - mac_old[port].FramesTransmittedOK
        return nb_rx, nb_tx

    def __find_stable_throughput(
        self,
        tg_controller: TofinoTGController,
        pktgen: Pktgen,
        churn: int,
        search_steps: int,
    ) -> ThroughputReport:
        self.log("Warming up ports...")
        self.warmup_ports(tg_controller, pktgen)
        sleep(REST_TIME_SEC)

        rate_lower = MIN_THROUGHPUT
        rate_upper = MAX_THROUGHPUT

        winner_report = ThroughputReport(
            requested_bps=0,
            pktgen_bps=0,
            pktgen_pps=0,
            dut_ingress_bps=0,
            dut_ingress_pps=0,
            dut_egress_bps=0,
            dut_egress_pps=0,
            loss=1,
        )

        current_rate = rate_upper

        # We iteratively refine the bounds until the difference between them is less than the specified precision.
        for i in range(search_steps):
            # There's no point in continuing this search.
            if rate_upper == 0 or current_rate == 0:
                break

            self.log()
            self.log(f"[{i+1}/{search_steps}] Trying rate {current_rate:,} Mbps")

            pktgen.set_churn(0)
            pktgen.set_rate(WARMUP_RATE)
            pktgen.activate_warmup_mode()
            pktgen.start()

            self.log(f"Warming up DUT...")
            loss = 1
            warmup_retries = -1
            while loss > MAX_ACCEPTABLE_LOSS:
                warmup_retries += 1
                if warmup_retries > MAX_WARMUP_RETRIES:
                    self.log("Max warmup retries reached! Power through...")
                    break

                measured_ports = set(tg_controller.broadcast_ports) | set(tg_controller.symmetric_ports)
                mac_old = tg_controller.get_port_stats_from_meta_table()
                sleep(WARMUP_TIME_SEC)
                mac_new = tg_controller.get_port_stats_from_meta_table()

                nb_rx_pkts, nb_tx_pkts = self._mac_pkt_deltas(mac_old, mac_new, measured_ports)

                loss = (1 - nb_rx_pkts / nb_tx_pkts) if nb_tx_pkts else 1
                self.log(f"[{warmup_retries:02}/{MAX_WARMUP_RETRIES:02}] Warmup TX pkts: {nb_tx_pkts:,}, RX pkts: {nb_rx_pkts:,}, loss: {loss*100:.3f}%")

            pktgen.deactivate_warmup_mode()
            pktgen.set_rate(current_rate)
            pktgen.set_churn(churn)
            sleep(REST_TIME_SEC)
            measured_ports = set(tg_controller.broadcast_ports) | set(tg_controller.symmetric_ports)
            mac_old = tg_controller.get_port_stats_from_meta_table()
            pktgen.reset_stats()
            sleep(ITERATION_DURATION_SEC)
            pktgen.stop()
            sleep(REST_TIME_SEC)
            mac_new = tg_controller.get_port_stats_from_meta_table()

            nb_rx_pkts, nb_tx_pkts = self._mac_pkt_deltas(mac_old, mac_new, measured_ports)

            pktgen_stats = pktgen.get_stats()
            pktgen_nb_tx_pkts = pktgen_stats[0]
            pktgen_nb_tx_bytes = pktgen_stats[1]

            pkt_size_without_crc = pktgen_nb_tx_bytes / pktgen_nb_tx_pkts
            pkt_size_with_crc = pkt_size_without_crc + 4  # 4 bytes CRC
            pktgen_nb_tx_bits = pktgen_nb_tx_pkts * pkt_size_with_crc * 8

            # Derive DUT bit rates from the reliable MAC frame counts and the wire
            # packet size measured by pktgen (echo/forwarding preserves packet size).
            nb_tx_bits = nb_tx_pkts * pkt_size_with_crc * 8
            nb_rx_bits = nb_rx_pkts * pkt_size_with_crc * 8

            report = ThroughputReport(
                requested_bps=current_rate * 1_000_000,
                pktgen_bps=int(pktgen_nb_tx_bits / ITERATION_DURATION_SEC),
                pktgen_pps=int(pktgen_nb_tx_pkts / ITERATION_DURATION_SEC),
                dut_ingress_bps=int(nb_tx_bits / ITERATION_DURATION_SEC),
                dut_ingress_pps=int(nb_tx_pkts / ITERATION_DURATION_SEC),
                dut_egress_bps=int(nb_rx_bits / ITERATION_DURATION_SEC),
                dut_egress_pps=int(nb_rx_pkts / ITERATION_DURATION_SEC),
                loss=(1 - nb_rx_pkts / nb_tx_pkts) if nb_tx_pkts else 1,
            )

            tx_Gbps = report.dut_ingress_bps / 1e9
            tx_Mpps = report.dut_ingress_pps / 1e6

            rx_Gbps = report.dut_egress_bps / 1e9
            rx_Mpps = report.dut_egress_pps / 1e6

            self.log(f"Pktgen {report.pktgen_pps / 1e6:12.5f} Mpps {report.pktgen_bps / 1e9:12.5f} Gbps")
            self.log(f"TX     {nb_tx_pkts:12} pkts {tx_Mpps:12.5f} Mpps {tx_Gbps:12.5f} Gbps")
            self.log(f"RX     {nb_rx_pkts:12} pkts {rx_Mpps:12.5f} Mpps {rx_Gbps:12.5f} Gbps")
            self.log(f"Lost   {report.loss*100:.3f}% of packets")

            if report.loss > MAX_ACCEPTABLE_LOSS:
                rate_upper = current_rate

                if current_rate == MIN_THROUGHPUT:
                    self.log("Lower bound reached, stopping search.")
                    break

                # Initial phase, let's quickly find the correct order of magnitude
                if rate_lower == MIN_THROUGHPUT:
                    current_rate = int(current_rate / 10)
                    continue
            else:
                winner_report = report
                rate_lower = current_rate
                if current_rate == rate_upper:
                    break

            current_rate = int((rate_upper + rate_lower) / 2)

        self.log()
        self.log(f"Winner {winner_report.dut_egress_pps / 1e6:12.5f} Mpps {winner_report.dut_egress_bps / 1e9:12.5f} Gbps")

        return winner_report

    def find_stable_throughput(
        self,
        tg_controller: TofinoTGController,
        pktgen: Pktgen,
        churn: int,
        search_steps: int = DEFAULT_THROUGHPUT_SEARCH_STEPS,
    ) -> ThroughputReport:
        report = self.__find_stable_throughput(
            tg_controller=tg_controller,
            pktgen=pktgen,
            churn=churn,
            search_steps=search_steps,
        )

        # Not really a great ideia, because (1) we take longer to run experiments,
        # and (2) sometimes the application behaves so badly that we can't even pass the
        # warmup phase, and we get stuck in a loop. Might as well move on.

        # bogus_retry = 0
        # while report.requested_bps == 0:
        #     if bogus_retry > BOGUS_RETRIES:
        #         self.log("Bogus retry limit reached, stopping search.")
        #         break

        #     self.log("That was probably a bogus attempt, trying again...")

        #     report = self.__find_stable_throughput(
        #         tg_controller=tg_controller,
        #         pktgen=pktgen,
        #         churn=churn,
        #         search_steps=search_steps,
        #     )

        #     bogus_retry += 1

        return report


class ExperimentTracker:
    def __init__(self) -> None:
        self.overall_progress = Progress(
            TimeElapsedColumn(),
            BarColumn(),
            TimeRemainingColumn(),
            TextColumn("{task.description}"),
        )

        self.experiment_iters_progress = Progress(
            TextColumn("  "),
            TextColumn("[bold blue]{task.fields[name]}: " "{task.percentage:.0f}%"),
            BarColumn(),
            TimeRemainingColumn(),
            TextColumn("{task.description}"),
        )

        self.step_progress = Progress(
            TextColumn("  "),
            TimeElapsedColumn(),
            TextColumn("[bold purple]"),
            BarColumn(),
            TimeRemainingColumn(),
            TextColumn("{task.description}"),
        )

        self.progress_group = Group(
            Group(self.step_progress, self.experiment_iters_progress),
            self.overall_progress,
        )

        self.experiments: list[Experiment] = []

    def add_experiment(self, experiment: Experiment) -> None:
        self.experiments.append(experiment)

    def add_experiments(self, experiments: list[Experiment]) -> None:
        self.experiments += experiments

    def run_experiments(self):
        with Live(self.progress_group):
            nb_exps = len(self.experiments)
            overall_task_id = self.overall_progress.add_task("", total=nb_exps)

            for i, exp in enumerate(self.experiments):
                description = f"[bold #AAAAAA]({i} out of {nb_exps} experiments)"
                self.overall_progress.update(overall_task_id, description=description)
                exp.run_many(self.experiment_iters_progress, self.step_progress)
                self.overall_progress.update(overall_task_id, advance=1)

            self.overall_progress.update(overall_task_id, description="[bold green] All done!")
