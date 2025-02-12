#!/usr/bin/env python3

from pathlib import Path
from typing import Optional

from .remote import RemoteHost
from .switch import Switch

TRAFFIC_GENERATOR_P4_NAME = "traffic-generator"

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
        self.src_in_repo = Path("tofino") / "traffic-generator" / f"{TRAFFIC_GENERATOR_P4_NAME}.p4"

    def install(self) -> None:
        super().install(self.src_in_repo)
    
    def launch(self) -> None:
        self.host.run_command("sudo killall bf_switchd")

        cmd = f"./run_switchd.sh -p {TRAFFIC_GENERATOR_P4_NAME}"
        self.controller_cmd = self.host.run_command(cmd, dir=self.sde)
        self.ready = False

class TofinoTGController:
    def __init__(
        self,
        hostname: str,
        repo: str,
        log_file: Optional[str] = None,
    ) -> None:
        self.host = RemoteHost(hostname, log_file=log_file)
        self.repo = Path(repo)
        # self.ports = ports
        self.exe = None

        self.host.test_connection()
        
        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")
        
        self.ready = False