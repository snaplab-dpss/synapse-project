#!/usr/bin/env python3

from pathlib import Path
from typing import Optional

from .remote import RemoteHost

class Switch:
    def __init__(
        self,
        hostname: str,
        repo: str,
        tofino_version: int,
        log_file: Optional[str] = None,
    ) -> None:
        self.host = RemoteHost(hostname, log_file=log_file)
        self.repo = Path(repo)
        self.tofino_version = tofino_version

        self.host.test_connection()
        
        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")

    def install(self, src: Path) -> None:
        assert self.host.remote_file_exists(src)

        program_name = src.stem
        makefile = self.repo / "tools" / "Makefile"

        if self.tofino_version == 1:
            make_target = "install-tofino1"
        elif self.tofino_version == 2:
            make_target = "install-tofino2"
        else:
            self.host.crash(f"We are only compatible with Tofino 1 and 2.")

        compilation_cmd = f"APP={program_name} make -f {makefile} {make_target}"

        cmd = self.host.run_command(compilation_cmd)
        cmd.watch()
