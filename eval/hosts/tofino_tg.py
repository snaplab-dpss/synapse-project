#!/usr/bin/env python3

import re
import time

from pathlib import Path
from typing import Optional, Tuple
from dataclasses import dataclass

from .remote import RemoteHost
from .switch import Switch

from .pktgen import Pktgen

APP_NAME = "traffic-generator"


@dataclass
class MetaPortStats:
    FramesReceivedOK: int
    FramesTransmittedOK: int


@dataclass
class PortStats:
    rx_pkts: int
    rx_bytes: int
    tx_pkts: int
    tx_bytes: int


@dataclass
class PortState:
    enable: bool
    up: bool


class TofinoTG(Switch):
    def __init__(
        self,
        hostname: str,
        repo: str,
        sde: str,
        tofino_version: int,
        log_file: Optional[str] = None,
    ) -> None:
        self.app_name = APP_NAME
        super().__init__(hostname, repo, sde, tofino_version, log_file)

    def install(self) -> None:
        src_in_repo = Path("tofino") / self.app_name / f"{self.app_name}.p4"
        super().install(src_in_repo)

    def launch(self) -> None:
        super().launch(self.app_name)


class TofinoTGController:
    def __init__(
        self,
        hostname: str,
        repo: str,
        sde: str,
        log_file: Optional[str] = None,
    ) -> None:
        self.host = RemoteHost(hostname, log_file=log_file)
        self.repo = Path(repo)
        self.sde = Path(sde)
        self.app_name = APP_NAME
        self.broadcast_ports = []
        self.symmetric_ports = []

        sde_install = f"{self.sde}/install"
        python_path: Optional[str] = None

        self.host.test_connection()

        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")

        sde_python_packages_options = self.host.find("site-packages", Path(sde_install))
        for sde_python_packages in sde_python_packages_options:
            self.host.log(f"Found SDE python packages candidate in {sde_python_packages}")
            tofino_packages = sde_python_packages / "tofino"
            if self.host.remote_dir_exists(tofino_packages):
                self.host.log(f"Candidate chosen!")
                python_path = f"{tofino_packages}:{sde_python_packages}"
                break

        if python_path is None:
            self.host.crash("Unable to find SDE python packages")

        self.env_vars = " ".join(
            [
                f"SDE={self.sde}",
                f"SDE_INSTALL={sde_install}",
                f"PYTHONPATH={python_path}",
            ]
        )

    def setup(
        self,
        broadcast: list[int],
        symmetric: list[int],
        route: list[Tuple[int, int]],
    ) -> None:
        self.broadcast_ports = broadcast
        self.symmetric_ports = symmetric

        target_dir = self.repo / "tofino" / self.app_name

        cmd = f"{self.env_vars} ./{self.app_name}.py setup set"
        if broadcast:
            cmd += f" --broadcast {' '.join(map(str, broadcast))}"
        if symmetric:
            cmd += f" --symmetric {' '.join(map(str, symmetric))}"
        for src, dst in route:
            cmd += f" --route {src} {dst}"

        command = self.host.run_command(cmd, dir=target_dir)
        command.watch()
        code = command.recv_exit_status()

        if code != 0:
            self.host.crash(f"{self.app_name} controller exited with code != 0.")
            exit(1)

    def get_ports_state(self) -> dict[int, PortState]:
        target_dir = self.repo / "tofino" / self.app_name

        cmd = f"{self.env_vars} ./{self.app_name}.py setup get"

        command = self.host.run_command(cmd, dir=target_dir)
        output = command.watch()
        self.host.log(output)
        code = command.recv_exit_status()

        if code != 0:
            self.host.crash(f"{self.app_name} controller exited with code != 0.")
            exit(1)

        state = {}
        start_parsing = False
        for line in output.splitlines():
            if not start_parsing:
                if line == "====== Port state report ======":
                    start_parsing = True
                continue

            result = re.search(r"(\d+): speed=(.+), fec=(.+), loopback_mode=(.+), port_enable=(\d), port_up=(\d).*", line)

            if not result:
                continue

            port = int(result.group(1))
            port_state = PortState(
                enable=bool(int(result.group(5))),
                up=bool(int(result.group(6))),
            )

            self.host.log(f"Port {port} state: {port_state}")

            state[port] = port_state

        return state

    def are_all_ports_ready(self) -> bool:
        port_state = self.get_ports_state()
        return all([state.up for state in port_state.values()])

    def wait_for_ports(self) -> None:
        while not self.are_all_ports_ready():
            self.host.log("Not all ports are up, waiting 5s and retrying...")
            time.sleep(5)

    def get_port_stats_from_meta_table(self) -> dict[int, MetaPortStats]:
        target_dir = self.repo / "tofino" / self.app_name

        cmd = f"{self.env_vars} ./{self.app_name}.py stats get --meta"

        command = self.host.run_command(cmd, dir=target_dir)
        output = command.watch()
        self.host.log(output)
        code = command.recv_exit_status()

        if code != 0:
            self.host.crash(f"{self.app_name} controller exited with code != 0.")
            exit(1)

        stats = {}
        start_parsing = False
        for line in output.splitlines():
            if not start_parsing:
                if line == "====== Stats report ======":
                    start_parsing = True
                continue

            result = re.search(r"(\d+):(\d+):(\d+)", line)
            if not result:
                continue

            port = int(result.group(1))
            FramesReceivedOK = int(result.group(2))
            FramesTransmittedOK = int(result.group(3))

            port_stats = MetaPortStats(FramesReceivedOK, FramesTransmittedOK)
            stats[port] = port_stats

        return stats

    def get_port_stats(self) -> dict[int, PortStats]:
        target_dir = self.repo / "tofino" / self.app_name

        cmd = f"{self.env_vars} ./{self.app_name}.py stats get"

        command = self.host.run_command(cmd, dir=target_dir)
        output = command.watch()
        self.host.log(output)
        code = command.recv_exit_status()

        if code != 0:
            self.host.crash(f"{self.app_name} controller exited with code != 0.")
            exit(1)

        stats = {}
        start_parsing = False
        for line in output.splitlines():
            if not start_parsing:
                if line == "====== Stats report ======":
                    start_parsing = True
                continue

            port, rx_pkts, rx_bytes, tx_pkts, tx_bytes = map(int, line.split(":"))
            port_stats = PortStats(rx_pkts, rx_bytes, tx_pkts, tx_bytes)
            stats[port] = port_stats

        return stats

    def reset_stats(self) -> None:
        target_dir = self.repo / "tofino" / self.app_name

        cmd = f"{self.env_vars} ./{self.app_name}.py stats clear"

        command = self.host.run_command(cmd, dir=target_dir)
        command.watch()
        code = command.recv_exit_status()

        if code != 0:
            self.host.crash(f"{self.app_name} controller exited with code != 0.")
            exit(1)

    def reset_to_default_acceleration(self) -> None:
        pass

    def set_acceleration(self, multiplier: int) -> None:
        pass

    def get_pktgen_rate_for_true_rate(self, rate: int) -> int:
        return rate
