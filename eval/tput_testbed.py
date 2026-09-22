#!/usr/bin/env python3
"""What each traffic generator can push, per packet size.

This measures the testbed, not an NF. The DUT runs the hand-written traffic generator with plain
echo rules -- every packet straight back out the port it arrived on -- so it is a mirror of known
throughput, and whatever the numbers show is the generator's doing. Both generators are measured:
the Tofino TG, and the hardware traffic accelerator.

Its sibling eval/test_baseline.py asks the other question, at one packet size: whether a given
datapath loses packets. Same mirror, different unknown.
"""

import argparse
import tomli

from pathlib import Path

from experiments.tput import ThroughputHosts
from experiments.tput_per_pkt_sz import ThroughputPerPacketSize
from experiments.experiment import ExperimentTracker
from hosts.tofino_tg import TofinoTG, TofinoTGController
from utils.kill_hosts import kill_hosts_on_sigint
from utils.constants import *

PKT_SIZES = [64, 128, 256, 512, 1024, 1280, 1500]


def echo_routes(client_ports: list[int]) -> list[tuple[int, int]]:
    """A -> A: send every packet back out the port it arrived on."""
    return [(p, p) for p in client_ports]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")

    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    kill_hosts_on_sigint(config)

    log_file = config["logs"]["experiment"]
    dut = config["devices"]["switch_dut"]
    client_ports = list(dut["client_ports"])

    # The DUT runs the traffic generator, so it is stood up the way the TG switch is rather than
    # through ThroughputHosts, which builds a DUT for a synthesized NF.
    dut_switch = TofinoTG(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=dut["sde"],
        tofino_version=dut["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    )

    dut_controller = TofinoTGController(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=dut["sde"],
        log_file=config["logs"]["controller_dut"],
    )

    exp_tracker = ExperimentTracker()

    hosts_per_generator = {
        "TG": ThroughputHosts(config, use_accelerator=False),
        "TA": ThroughputHosts(config, use_accelerator=True),
    }

    for generator, hosts in hosts_per_generator.items():
        exp_tracker.add_experiment(
            ThroughputPerPacketSize(
                name=f"Echo tput/pkt_sz ({generator})",
                save_name=DATA_DIR / f"tput_testbed_{generator.lower()}.csv",
                pkt_sizes=PKT_SIZES,
                hosts=hosts,
                dut_switch=dut_switch,
                dut_controller=dut_controller,
                nb_flows=10_000,
                broadcast=client_ports,
                # The DUT's own broadcast group goes unused -- the echo rules do all the
                # forwarding -- but naming the client ports keeps it from being empty.
                dut_broadcast=client_ports,
                dut_route=echo_routes(client_ports),
                experiment_log_file=log_file,
            )
        )

    exp_tracker.run_experiments()

    for hosts in hosts_per_generator.values():
        hosts.terminate()


if __name__ == "__main__":
    main()
