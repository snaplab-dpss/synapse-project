#!/usr/bin/env python3

import re
import time

from pathlib import Path
from typing import Optional, Tuple

from .remote import RemoteHost
from .switch import Switch

APP_NAME = "switcharoo"


class Switcharoo(Switch):
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
        src_in_repo = Path("tofino") / self.app_name / "p4" / f"{self.app_name}.p4"
        super().install(src_in_repo)

    def launch(self) -> None:
        super().launch(self.app_name)


class SwitcharooController:
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

    def setup(self) -> None:
        target_dir = self.repo / "tofino" / self.app_name

        cmd = f"{self.env_vars} ./{self.app_name}.py"

        command = self.host.run_command(cmd, dir=target_dir)
        command.watch()
        code = command.recv_exit_status()

        if code != 0:
            self.host.crash(f"{self.app_name} controller exited with code != 0.")
            exit(1)
