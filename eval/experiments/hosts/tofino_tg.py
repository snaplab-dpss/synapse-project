#!/usr/bin/env python3

from pathlib import Path
from typing import Optional, Tuple

from .remote import RemoteHost
from .switch import Switch

TRAFFIC_GENERATOR_APP_NAME = "traffic-generator"

class TofinoTG(Switch):
    def __init__(
        self,
        hostname: str,
        repo: str,
        sde: str,
        tofino_version: int,
        log_file: Optional[str] = None,
    ) -> None:
        super().__init__(hostname, repo, sde, tofino_version, log_file)
        self.ready = False
        self.switchd_cmd = None

    def install(self) -> None:
        src_in_repo = Path("tofino") / "traffic-generator" / f"{TRAFFIC_GENERATOR_APP_NAME}.p4"
        super().install(src_in_repo)
    
    def launch(self) -> None:
        if self.ready:
            return
        
        if self.host.is_proc_running("bf_switchd"):
            self.kill_switchd()

        env_vars = " ".join([
            f"SDE={self.sde}",
            f"SDE_INSTALL={self.sde}/install",
        ])

        cmd = f"{env_vars} ./run_switchd.sh -p {TRAFFIC_GENERATOR_APP_NAME}"
        self.switchd_cmd = self.host.run_command(cmd, dir=self.sde)
        self.ready = False
    
    def wait_ready(self) -> None:
        if self.ready:
            return

        if self.switchd_cmd is None:
            raise RuntimeError("Switchd not started")

        self.switchd_cmd.watch(
            stop_pattern="bfshell> ",
        )

        if self.switchd_cmd.exit_status_ready():
            self.host.crash("Switchd exited before time.")

        self.ready = True
    
    def kill_switchd(self) -> None:
        self.host.run_command("sudo killall bf_switchd")
        self.switchd_cmd = None
        self.ready = False

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

        self.host.test_connection()
        
        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")
        
    def run(
            self,
            broadcast: list[int],
            symmetric: list[int],
            route: list[Tuple[int,int]],
    ) -> None:
        target_dir = self.repo / "tofino" / "traffic-generator"

        env_vars = " ".join([
            f"SDE={self.sde}",
            f"SDE_INSTALL={self.sde}/install",
            f"PYTHONPATH={self.sde}/install/lib/python3.5/site-packages/tofino:{self.sde}/install/lib/python3.5/site-packages/"
        ])

        cmd = f"{env_vars} ./{TRAFFIC_GENERATOR_APP_NAME}.py"
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
            self.host.crash(f"Tofino TG controller exited with code != 0.")
