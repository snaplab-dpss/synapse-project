import time

from datetime import datetime
from typing import Optional
from pathlib import Path

from rich.console import Group
from rich.live import Live
from rich.progress import (
    BarColumn,
    Progress,
    TextColumn,
    TimeElapsedColumn,
    TimeRemainingColumn,
)

from experiments.hosts.pktgen import Pktgen

MAX_THROUGHPUT          = 100_000 # 100 Gbps
ITERATION_DURATION_SEC  = 5       # seconds
THROUGHPUT_SEARCH_STEPS = 10
MAX_ACCEPTABLE_LOSS     = 0.001   # 0.1%
WARMUP_TIME_SEC         = 5       # seconds
WARMUP_RATE             = 1       # 1 Mbps
REST_TIME_SEC           = 2       # seconds

# FIXME: Change this
EXPERIMENT_ITERATIONS   = 3

class Experiment:
    def __init__(
        self,
        name: str,
        log_file: Optional[str] = None,
    ) -> None:
        self.name = name

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
        
    def log(self, msg=""):
        if self.log_file:
            now = datetime.now()
            ts = now.strftime('%Y-%m-%d %H:%M:%S')
            print(f"[{ts}][{self.name}] {msg}", file=self.log_file, flush=True)

    def run(self, step_progress: Progress, current_iter: int) -> None:
        raise NotImplementedError

    def run_many(self, progress: Progress, step_progress: Progress) -> None:
        task_id = progress.add_task("", total=EXPERIMENT_ITERATIONS, name=self.name)
        for iter in range(EXPERIMENT_ITERATIONS):
            self.run(step_progress, iter)
            progress.update(task_id, advance=1)

        progress.update(task_id, description="[bold green] done!")
    
    def test_throughput(
        self,
        rate_mbps: int,
        pktgen: Pktgen,
        churn: int,
        pkt_size: int,
    ) -> tuple[int, int, float]:
        if not (0 <= MAX_ACCEPTABLE_LOSS < 1):
            raise ValueError("max_loss must be in [0, 1).")

        # Setting the churn value
        pktgen.set_churn(churn)

        # Setting warmup duration
        pktgen.set_warmup_duration(WARMUP_TIME_SEC)

        self.log()
        self.log(f"Rate {rate_mbps:,} Mbps")

        nb_tx_pkts = 0
        nb_rx_pkts = 0

        while nb_tx_pkts == 0:
            pktgen.reset_stats()
            pktgen.set_rate(rate_mbps)
            
            # Run pktgen with warmup
            pktgen.run(ITERATION_DURATION_SEC)
            
            # Let the flows expire.
            time.sleep(REST_TIME_SEC)

            nb_tx_pkts, nb_rx_pkts = pktgen.get_stats()

            if nb_tx_pkts == 0:
                self.log(f"No packets flowing, repeating run")

        nb_tx_bits = nb_tx_pkts * (pkt_size + 20) * 8
        nb_rx_bits = nb_rx_pkts * (pkt_size + 20) * 8

        real_throughput_tx_bps = int(nb_tx_bits / ITERATION_DURATION_SEC)
        real_throughput_tx_pps = int(nb_tx_pkts / ITERATION_DURATION_SEC)

        real_throughput_rx_bps = int(nb_rx_bits / ITERATION_DURATION_SEC)
        real_throughput_rx_pps = int(nb_rx_pkts / ITERATION_DURATION_SEC)

        loss = 1 - nb_rx_pkts / nb_tx_pkts

        tx_Gbps = real_throughput_tx_bps / 1e9
        tx_Mpps = real_throughput_tx_pps / 1e6

        rx_Gbps = real_throughput_rx_bps / 1e9
        rx_Mpps = real_throughput_rx_pps / 1e6

        self.log(f"TX {tx_Mpps:.2f} Mpps {tx_Gbps:.2f} Gbps")
        self.log(f"RX {rx_Mpps:.2f} Mpps {rx_Gbps:.2f} Gbps")
        self.log(f"Lost {loss*100:.2f}% of packets")

        return real_throughput_tx_bps, real_throughput_tx_pps, loss
    
    def find_stable_throughput(
        self,
        pktgen: Pktgen,
        churn: int,
        pkt_size: int,
    ) -> tuple[int,int,int]:        
        if not (0 <= MAX_ACCEPTABLE_LOSS < 1):
            raise ValueError("max_loss must be in [0, 1).")

        rate_lower = 0
        rate_upper = MAX_THROUGHPUT

        real_throughput_bps_winner = 0
        real_throughput_pps_winner = 0
        requested_rate_mbps_winner = 0

        current_rate = rate_upper

        # Setting the churn value
        pktgen.set_churn(churn)

        # Setting warmup duration
        pktgen.set_warmup_duration(WARMUP_TIME_SEC)

        self.log()

        # We iteratively refine the bounds until the difference between them is
        # less than the specified precision.
        for i in range(THROUGHPUT_SEARCH_STEPS):
            # There's no point in continuing this search.
            if rate_upper == 0:
                break

            self.log(f"[{i+1}/{THROUGHPUT_SEARCH_STEPS}] Trying rate {current_rate:,} Mbps")

            nb_tx_pkts = 0
            nb_rx_pkts = 0

            while nb_tx_pkts == 0:
                pktgen.reset_stats()
                pktgen.set_rate(current_rate)
                
                # Run pktgen with warmup
                pktgen.run(ITERATION_DURATION_SEC)
                
                # Let the flows expire.
                time.sleep(REST_TIME_SEC)

                nb_tx_pkts, nb_rx_pkts = pktgen.get_stats()

                if nb_tx_pkts == 0:
                    self.log(f"No packets flowing, repeating run")

            nb_tx_bits = nb_tx_pkts * (pkt_size + 20) * 8
            nb_rx_bits = nb_rx_pkts * (pkt_size + 20) * 8

            real_throughput_tx_bps = int(nb_tx_bits / ITERATION_DURATION_SEC)
            real_throughput_tx_pps = int(nb_tx_pkts / ITERATION_DURATION_SEC)

            real_throughput_rx_bps = int(nb_rx_bits / ITERATION_DURATION_SEC)
            real_throughput_rx_pps = int(nb_rx_pkts / ITERATION_DURATION_SEC)

            loss = 1 - nb_rx_pkts / nb_tx_pkts

            tx_Gbps = real_throughput_tx_bps / 1e9
            tx_Mpps = real_throughput_tx_pps / 1e6

            rx_Gbps = real_throughput_rx_bps / 1e9
            rx_Mpps = real_throughput_rx_pps / 1e6

            self.log(f"TX {tx_Mpps:.5f} Mpps {tx_Gbps:.5f} Gbps")
            self.log(f"RX {rx_Mpps:.5f} Mpps {rx_Gbps:.5f} Gbps")
            self.log(f"Lost {loss*100}% of packets")

            if loss > MAX_ACCEPTABLE_LOSS:
                rate_upper = current_rate

                # Initial phase, let's quickly find the correct order of magnitude
                if rate_lower == 0:
                    current_rate = int(current_rate / 10)
                    continue
            else:
                if current_rate == rate_upper:
                    return real_throughput_tx_bps, real_throughput_tx_pps, current_rate

                rate_lower = current_rate

                real_throughput_bps_winner = real_throughput_tx_bps
                real_throughput_pps_winner = real_throughput_tx_pps
                requested_rate_mbps_winner = current_rate

            current_rate = int((rate_upper + rate_lower) / 2)

        # Found a rate.
        return real_throughput_bps_winner, real_throughput_pps_winner, requested_rate_mbps_winner

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
            TextColumn(
                "[bold blue]{task.fields[name]}: " "{task.percentage:.0f}%"
            ),
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
                description = (
                    f"[bold #AAAAAA]({i} out of {nb_exps} experiments)"
                )
                self.overall_progress.update(
                    overall_task_id, description=description
                )
                exp.run_many(
                    self.experiment_iters_progress, self.step_progress
                )
                self.overall_progress.update(overall_task_id, advance=1)

            self.overall_progress.update(
                overall_task_id, description="[bold green] All done!"
            )
