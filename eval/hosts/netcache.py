#!/usr/bin/env python3

import re

from pathlib import Path
from typing import Optional

from .remote import RemoteHost
from .switch import Switch

APP_NAME = "netcache"


class NetCache(Switch):
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


class NetCacheController:
    def __init__(
        self,
        hostname: str,
        repo: str,
        sde: str,
        tofino_version: int,
        client_ports: list[int],
        server_port: int,
        log_file: Optional[str] = None,
    ) -> None:
        self.host = RemoteHost(hostname, log_file=log_file)
        self.repo = Path(repo)
        self.sde = Path(sde)
        self.tofino_version = tofino_version
        self.client_ports = client_ports
        self.server_port = server_port
        self.app_name = APP_NAME

        self.path_to_controller = self.repo / "tofino" / "netcache" / "controller"

        self.host.test_connection()

        if not self.host.remote_dir_exists(self.path_to_controller):
            self.host.crash(f"NetCache controller not found on remote host {self.host}")

        if not self.host.remote_dir_exists(self.sde):
            self.host.crash(f"SDE directory not found on remote host {self.sde}")

        self._compile()

        self.ready = False

    def _clean(self) -> None:
        build_dir = self.path_to_controller / "build"

        compilation_cmd = f"rm -rf {build_dir}"
        cmd = self.host.run_command(compilation_cmd, dir=self.path_to_controller)
        cmd.watch()
        code = cmd.recv_exit_status()

        if code != 0:
            self.host.crash(f"NetCache controller cleanup failed.")

    def _compile(self) -> None:
        build_script = self.path_to_controller / "build.sh"

        env_vars = " ".join(
            [
                f"SDE={self.sde}",
                f"SDE_INSTALL={self.sde}/install",
            ]
        )

        compilation_cmd = f"{env_vars} {build_script}"
        cmd = self.host.run_command(compilation_cmd, dir=self.path_to_controller)
        cmd.watch()
        code = cmd.recv_exit_status()

        if code != 0:
            self.host.crash(f"NetCache compilation failed.")

    def launch(
        self,
        disable_cache: bool = False,
    ) -> None:
        self._compile()

        config_file = self.path_to_controller / "conf" / "conf.json"

        if not self.host.remote_file_exists(config_file):
            self.host.crash(f"NetCache config file {config_file} not found on remote host {self.host}")

        cmd = f"sudo -E ./build/Release/netcache-controller"
        cmd += f" --config {config_file}"
        cmd += f" --tna {self.tofino_version}"
        cmd += f" --wait-ports"
        cmd += f" --client-ports {' '.join(map(str, self.client_ports))}"
        cmd += f" --server-port {self.server_port}"

        if disable_cache:
            cmd += " --disable-cache"

        self.controller_cmd = self.host.run_command(cmd, dir=self.path_to_controller)
        self.ready = False

    def wait_ready(self) -> None:
        if self.ready:
            # We already checked that it's ready
            return

        if self.controller_cmd is None:
            raise RuntimeError("Controller not started")

        self.controller_cmd.watch(
            stop_pattern=re.escape("NetCache controller is ready."),
        )

        if self.controller_cmd.exit_status_ready():
            self.host.crash("Controller exited unexpectedly")

        self.ready = True

    def kill_controller(self) -> None:
        self.host.run_command("sudo killall netcache-controller")
        self.switchd_cmd = None
        self.ready = False
