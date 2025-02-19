#!/usr/bin/env python3

from pathlib import Path
from typing import Optional, Tuple

from .remote import RemoteHost
from .switch import Switch

APP_NAME = "traffic-generator"

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

        self.host.test_connection()
        
        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")
        
    def run(
            self,
            broadcast: list[int],
            symmetric: list[int],
            route: list[Tuple[int,int]],
    ) -> None:
        target_dir = self.repo / "tofino" / self.app_name

        env_vars = " ".join([
            f"SDE={self.sde}",
            f"SDE_INSTALL={self.sde}/install",
            f"PYTHONPATH={self.sde}/install/lib/python3.5/site-packages/tofino:{self.sde}/install/lib/python3.5/site-packages/"
        ])

        cmd = f"{env_vars} ./{self.app_name}.py"
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
