#!/usr/bin/env python3

from paramiko import ssh_exception

from pathlib import Path
from typing import Optional

from .remote import RemoteHost


MIN_TIMEOUT = 10 # miliseconds

class Controller:
    def __init__(
        self,
        hostname: str,
        repo: str,
        sde_install: str,
        tofino_version: int,
        switch_pktgen_in_port: int,
        switch_pktgen_out_port: int,
        log_file: Optional[str] = None,
    ) -> None:
        self.host = RemoteHost(hostname, log_file=log_file)
        self.repo = Path(repo)
        self.sde_install = Path(sde_install)
        self.tofino_version = tofino_version
        self.switch_pktgen_in_port = switch_pktgen_in_port
        self.switch_pktgen_out_port = switch_pktgen_out_port
        self.controller_cmd = None
        self.exe = None

        self.host.test_connection()
        
        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")
        
        if not self.host.remote_dir_exists(self.sde_install):
            self.host.crash(f"SDE_INSTALL directory not found on remote host {self.sde_install}")
        
    def _compile(self, src: Path) -> None:
        if not self.host.remote_file_exists(src):
            self.host.crash(f"Unable to find file {src}")

        program_name = src.stem
        makefile = self.repo / "tools" / "Makefile"

        env_vars = f"SDE_INSTALL={self.sde_install} APP={program_name}"
        compilation_cmd = f"{env_vars} make -f {makefile} -j"
        cmd = self.host.run_command(compilation_cmd, dir=src.parent)
        code = cmd.recv_exit_status()

        if code != 0:
            self.host.crash(f"Controller compilation failed (app={program_name}).")

        self.controller_cmd = None

    def launch(
        self,
        src_in_repo: str,
        timeout_ms: int = MIN_TIMEOUT,
    ) -> None:
        if timeout_ms != 0 and timeout_ms < MIN_TIMEOUT:
            raise Exception(f"Timeout value must be 0 or >= {MIN_TIMEOUT}ms (is {timeout_ms}ms)")

        src_path = self.repo / src_in_repo
        self._compile(src_path)

        # The build script generates an executable with the same name as the source file
        # but without the extension.
        self.exe = src_path.stem

        cmd = f"sudo -E ./build/release/{self.exe}"
        cmd += f" --tna {self.tofino_version}"
        cmd += f" --wait-ports"
        cmd += f" --in {self.switch_pktgen_in_port}"
        cmd += f" --out {self.switch_pktgen_out_port}"
        cmd += f" --expiration-time {timeout_ms}"

        self.controller_cmd = self.host.run_command(cmd, dir=src_path.parent)

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
        try:
            self.stop()
        except ssh_exception.SSHException:
            # The ssh session is closed.
            pass
