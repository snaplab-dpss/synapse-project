#!/usr/bin/env python3

from signal import SIGINT, signal
from typing import Optional

from hosts.pktgen import Pktgen
from hosts.kvs_server import KVSServer
from hosts.netcache import NetCache, NetCacheController
from hosts.switch import Switch
from hosts.synapse import SynapseController
from utils.constants import *

config: Optional[dict] = None


def kill_hosts(config: dict) -> None:
    print("Killing DUT switch...")
    Switch(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    ).kill_switchd()

    print("Killing TG switch...")
    Switch(
        hostname=config["hosts"]["switch_tg"],
        repo=config["repo"]["switch_tg"],
        sde=config["devices"]["switch_tg"]["sde"],
        tofino_version=config["devices"]["switch_tg"]["tofino_version"],
        log_file=config["logs"]["switch_tg"],
    ).kill_switchd()

    print("Killing controller for Tofino DUT...")
    SynapseController(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["controller_dut"],
    ).stop()

    print("Killing pktgen...")
    Pktgen(
        hostname=config["hosts"]["pktgen"],
        repo=config["repo"]["pktgen"],
        rx_pcie_dev=config["devices"]["pktgen"]["rx_dev"],
        tx_pcie_dev=config["devices"]["pktgen"]["tx_dev"],
        nb_tx_cores=config["devices"]["pktgen"]["nb_tx_cores"],
        log_file=config["logs"]["pktgen"],
    ).kill_pktgen()

    print("Killing KVS server...")
    KVSServer(
        hostname=config["hosts"]["server"],
        repo=config["repo"]["server"],
        pcie_dev=config["devices"]["server"]["dev"],
        log_file=config["logs"]["server"],
    ).kill_server()

    print("Killing NetCache...")
    NetCache(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        log_file=config["logs"]["switch_dut"],
    ).kill_switchd()

    print("Killing controller for NetCache...")
    NetCacheController(
        hostname=config["hosts"]["switch_dut"],
        repo=config["repo"]["switch_dut"],
        sde=config["devices"]["switch_dut"]["sde"],
        tofino_version=config["devices"]["switch_dut"]["tofino_version"],
        client_ports=config["devices"]["switch_dut"]["client_ports"],
        server_port=config["devices"]["switch_dut"]["server_port"],
        log_file=config["logs"]["controller_dut"],
    ).kill_controller()

    print("Done!")


def signal_handler(sig, frame):
    if config is not None:
        kill_hosts(config)
    else:
        print("No config provided, therefore, not killing hosts")
    exit(1)


def kill_hosts_on_sigint(local_config: dict) -> None:
    global config
    config = local_config
    signal(SIGINT, signal_handler)
