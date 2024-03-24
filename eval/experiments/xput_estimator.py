#!/usr/bin/env python3

from experiments.hosts.controller import Controller
from experiments.hosts.switch import Switch
from experiments.hosts.pktgen import Pktgen

from experiments.experiment import Experiment

from pathlib import Path

from rich.console import Console
from rich.progress import Progress

from typing import NewType, Optional, Union

import itertools

# Controller argument name, flag, and value range
# E.g. ("r0", "--r0", [0, 0.5, 1])
# The argument name will show up on the csv file, while the flag will be used to pass the value to the controller.
Ratio = NewType("Ratio", tuple[str, str, list[float]])

class ThroughputEstimator(Experiment):
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
        ratios: list[Ratio],

        # Pktgen
        nb_flows: int,
        pkt_size: int,
        
        # Extra
        experiment_log_file: Optional[str] = None,
        console: Console = Console(),
        controller_extra_args: list[tuple[str, Union[str,int,float]]] = [],
    ) -> None:
        super().__init__(name, experiment_log_file)

        self.save_name = save_name

        self.switch = switch
        self.controller = controller
        self.pktgen = pktgen

        self.p4_src_in_repo = p4_src_in_repo
        
        self.controller_src_in_repo = controller_src_in_repo
        self.ratios = ratios
        self.controller_extra_args = controller_extra_args

        self.nb_flows = nb_flows
        self.pkt_size = pkt_size

        self.console = console

        self._sync()

    def _sync(self):
        rnames = [ r[0] for r in self.ratios ]
        header = f"#iteration, {', '.join(rnames)}, #asic pkts, #cpu pkts, throughput (bps), throughput (pps)\n"

        self.experiment_tracker = set()
        self.save_name.parent.mkdir(parents=True, exist_ok=True)

        # If file exists, continue where we left off.
        if self.save_name.exists():
            with open(self.save_name) as f:
                read_header = f.readline()
                assert header == read_header
                for row in f.readlines():
                    cols = row.split(",")
                    variables = [ float(c) for c in cols[:-4] ]
                    self.experiment_tracker.add((*variables,))
        else:
            with open(self.save_name, "w") as f:
                f.write(header)
    
    def _read_cpu_counters(
        self,
        ratios: tuple[float, ...],
        stable_rate_mbps: int,
    ) -> tuple[int, int]:
        rlabels = [ r[0] for r in self.ratios ]
        rdescriptions = " ".join(f"{rlabels[i]}={ratios[i]}" for i in range(len(ratios)))
        
        self.log(f"Reading CPU counters ({rdescriptions} rate={stable_rate_mbps:,} Mbps)")

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
        rvalues_range = [ r[2] for r in self.ratios ]
        all_rvalues = list(itertools.product(*rvalues_range))

        task_id = step_progress.add_task(self.name, total=len(all_rvalues))

        # Check if we already have everything before running all the programs.
        completed = True
        for rvalues in all_rvalues:
            exp_key = (current_iter,*rvalues,)
            if exp_key not in self.experiment_tracker:
                completed = False
                break
        if completed:
            return

        self.switch.install(self.p4_src_in_repo)

        self.pktgen.launch(
            self.nb_flows,
            pkt_size=self.pkt_size,
        )

        for rvalues in all_rvalues:
            exp_key = (current_iter,*rvalues,)

            rlabels = [ r[0] for r in self.ratios ]
            rflags = [ r[1] for r in self.ratios ]
            rdescriptions = " ".join(f"{rlabels[i]}={rvalues[i]}" for i in range(len(rvalues)))

            description=f"{self.name} (it={current_iter} {rdescriptions})"

            if exp_key in self.experiment_tracker:
                self.console.log(f"[orange1]Skipping: iteration {current_iter} ratios {rdescriptions}")
                step_progress.update(task_id, description=description, advance=1)
                continue

            step_progress.update(task_id, description=description)

            self.controller.launch(
                self.controller_src_in_repo,
                extra_args=self.controller_extra_args + [
                    (flag, value) for flag, value in zip(rflags, rvalues)
                ],
            )

            self.controller.wait_ready()
            self.pktgen.wait_launch()

            throughput_bps, throughput_pps, stable_thpt_mbps = self.find_stable_throughput(self.pktgen, 0, self.pkt_size)

            step_progress.update(
                task_id,
                description=description + " [reading CPU counters]"
            )

            self.log(f"Ratios {rdescriptions} => {int(throughput_bps/1e6):,} Mbps {int(throughput_pps/1e6):,} Mpps")
            in_pkts, cpu_pkts = self._read_cpu_counters(rvalues, stable_thpt_mbps)

            with open(self.save_name, "a") as f:
                rstrs = [ str(r) for r in rvalues ]
                f.write(f"{current_iter},{','.join(rstrs)},{in_pkts},{cpu_pkts},{throughput_bps},{throughput_pps}\n")

            step_progress.update(task_id, advance=1)
            self.controller.stop()

        self.pktgen.close()
        step_progress.update(task_id, visible=False)
