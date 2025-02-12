#!/usr/bin/env python3

from pathlib import Path
from typing import Optional

from .switch import Switch

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
        self.src_in_repo = Path("tofino") / "traffic-generator" / "traffic-generator.p4"

    def install(self) -> None:
        super().install(self.src_in_repo)
    
    def launch(self) -> None:
        self.host.run_command("sudo killall bf_switchd")

        src_path = self.repo / self.src_in_repo
        p4_name = src_path.stem
        cmd = f"./run_switchd.sh -p {p4_name}"

        self.controller_cmd = self.host.run_command(cmd, dir=self.sde)
        self.ready = False

class TofinoTGController:
    pass