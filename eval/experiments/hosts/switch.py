#!/usr/bin/env python3

from pathlib import Path
from typing import Optional

from .remote import RemoteHost

class Switch:
    def __init__(
        self,
        hostname: str,
        repo: str,
        sde: str,
        tofino_version: int,
        log_file: Optional[str] = None,
    ) -> None:
        self.host = RemoteHost(hostname, log_file=log_file)
        self.repo = Path(repo)
        self.sde = Path(sde)
        self.tofino_version = tofino_version

        self.host.test_connection()
        
        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")
        
        if not self.host.remote_dir_exists(self.sde):
            self.host.crash(f"SDE directory not found on remote host {self.sde}")

    def install(
        self,
        src_in_repo: str,
        compile_time_vars: list[tuple[str,str]] = [],
    ) -> None:
        src_path = self.repo / src_in_repo
        
        if not self.host.remote_file_exists(src_path):
            self.host.crash(f"Unable to find file {src_path}")

        program_name = src_path.stem
        makefile = self.repo / "tools" / "Makefile"

        compilation_vars = " ".join([ f"-D{key}={value}" for key, value in compile_time_vars ])
        
        if self.tofino_version == 1:
            make_target = "install-tofino1"
        elif self.tofino_version == 2:
            make_target = "install-tofino2"
        else:
            self.host.crash(f"We are only compatible with Tofino 1 and 2.")

        env_vars = " ".join([
            f"SDE={self.sde}",
            f"SDE_INSTALL={self.sde}/install",
            f"P4_COMPILATION_VARS=\"{compilation_vars}\"",
            f"CONTROLLER_ARGS=\"\"", # hack to ignore controller args
        ])

        compilation_cmd = f"{env_vars} make -f {makefile} {make_target}"
        cmd = self.host.run_command(compilation_cmd, dir=src_path.parent)
        cmd.watch()
        code = cmd.recv_exit_status()

        if code != 0:
            self.host.crash(f"P4 compilation failed (app={program_name}).")
