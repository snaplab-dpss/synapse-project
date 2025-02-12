#!/usr/bin/env python3

import argparse
import tomli

from pathlib import Path

from experiments.hosts.pktgen import Pktgen
from experiments.hosts.tofino_tg import TofinoTG, TofinoTGController
from experiments.hosts.switch import Switch
from experiments.hosts.synapse import SynapseController

def main():
    parser = argparse.ArgumentParser()

    parser.add_argument("-c", "--config-file", type=Path, default="experiment_config.toml", help="Path to config file")
    
    args = parser.parse_args()
    
    with open(args.config_file, "rb") as f:
        config = tomli.load(f)
    
    print("Connecting to Tofino DUT")
    dut_switch = Switch(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    )

    print("Connecting to Tofino DUT (controller)")
    synapse_controller = SynapseController(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        ports=config["devices"]["switch_dut"]["ports"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["controller_dut"],
    )

    print("Connecting to Tofino TG")
    tg_switch = TofinoTG(
        hostname=config["hosts"]["switch_tg"],
        repo=config["repo"]["switch_tg"],
        sde=config["devices"]["switch_tg"]["sde"],
        tofino_version=config["devices"]["switch_tg"]["tofino_version"],
        log_file=config["logs"]["switch_tg"],
    )

    print("Connecting to Tofino TG (controller)")
    tg_controller = TofinoTGController(
        hostname=config["hosts"]["switch_tg"],
        repo=config["repo"]["switch_tg"],
        sde=config["devices"]["switch_tg"]["sde"],
        log_file=config["logs"]["controller_tg"],
    )

    print("Connecting to pktgen")
    pktgen = Pktgen(
        hostname=config["hosts"]["pktgen"],
        repo=config["repo"]["pktgen"],
        rx_pcie_dev=config["devices"]["pktgen"]["rx_dev"],
        tx_pcie_dev=config["devices"]["pktgen"]["tx_dev"],
        nb_tx_cores=config["devices"]["pktgen"]["nb_tx_cores"],
        log_file=config["logs"]["pktgen"],
    )

    print("Installing Tofino TG")
    tg_switch.install()

    print("Launching Tofino TG")
    tg_switch.launch()
    tg_switch.wait_ready()

    print("Running Tofino TG (controller)")
    tg_controller.run(
        broadcast=[3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30],
        symmetric=[],
        route=[],
    )

    # print("Launching pktgen")
    # pktgen.launch(
    #     nb_flows=10_000,
    #     pkt_size=200,
    #     exp_time_us=1_000_000,
    #     crc_unique_flows=False,
    #     crc_bits=16,
    #     seed=0,
    #     mark_warmup_packets=False,
    # )
    # pktgen.wait_launch()

    print("Installing Tofino DUT")
    dut_switch.install(src_in_repo="tofino/forwarder/forwarder.p4")

    print("Launching Tofino DUT (controller)")
    synapse_controller.launch(src_in_repo="tofino/forwarder/forwarder.cpp")
    synapse_controller.wait_ready()

    print("Getting port stats")
    port_stats = synapse_controller.get_port_stats()
    print(port_stats)

    print("Resetting")
    synapse_controller.reset_port_stats()

    tg_switch.kill_switchd()
    synapse_controller.quit()

if __name__ == "__main__":
    main()
