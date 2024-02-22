#!/usr/bin/env python3

from pathlib import Path
from typing import Optional, Union

from .remote import RemoteHost

MIN_TIMEOUT = 10 # miliseconds

class Controller:
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
        self.controller_cmd = None
        self.exe = None

        self.host.test_connection()
        
        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")
        
    def __compile(self, src: Path) -> None:
        program_name = src.stem
        makefile = self.repo / "tools" / "Makefile"

        compilation_cmd = f"APP={program_name} make -f {makefile}"

        cmd = self.host.run_command(
            compilation_cmd,
            dir=str(src.parent)
        )
        
        cmd.watch()
        self.controller_cmd = None

    def launch(
        self,
        src: Path,
        switch_pktgen_in_port: int,
        switch_pktgen_out_port: int,
        timeout_ms: int = MIN_TIMEOUT,
    ) -> None:
        if timeout_ms < MIN_TIMEOUT:
            raise Exception(f"Timeout value must be >= {MIN_TIMEOUT}ms (is {timeout_ms}ms)")
        
        assert self.host.remote_file_exists(src)

        self.__compile(src)

        # The build script generates an executable with the same name as the source file
        # but without the extension.
        self.exe = src.stem

        cmd = f"sudo -E ./{self.exe}"
        cmd += f"--tna {self.tofino_version}"
        cmd += f"--wait-ports"
        cmd += f"--in {switch_pktgen_in_port}"
        cmd += f"--out {switch_pktgen_out_port}"
        cmd += f"--expiration-time {timeout_ms}"

        self.controller_cmd = self.host.run_command(
            cmd,
            dir=str(src.parent)
        )

    def wait_ready(self) -> None:
        # Wait for the controller to be ready.
        # It prints the message "Press enter to terminate." when it's ready.

        if self.controller_cmd is None:
            raise RuntimeError("Controller not started")

        self.controller_cmd.watch(
            stop_pattern="Press enter to terminate.",
        )

    def stop(self) -> None:
        if self.controller_cmd is None:
            return

        # Kill all instances
        cmd = f"killall {self.exe}"

        self.host.run_command(cmd)

        self.controller_cmd.watch()

        self.controller_cmd = None
        self.exe = None
    
    def __del__(self):
        self.stop()
