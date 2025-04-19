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
        self.makefile = self.repo / "tofino" / "tools" / "Makefile"
        self.ready = False
        self.switchd_cmd = None

        self.host.test_connection()

        assert self.tofino_version in [1, 2], "Tofino version must be 1 or 2"

        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")

        if not self.host.remote_dir_exists(self.sde):
            self.host.crash(f"SDE directory not found on remote host {self.sde}")

    def install(
        self,
        src_in_repo: str | Path,
        compile_time_vars: list[tuple[str, str]] = [],
    ) -> None:
        src_path = self.repo / src_in_repo

        if not self.host.remote_file_exists(src_path):
            self.host.crash(f"Unable to find file {src_path}")

        program_name = src_path.stem

        compilation_vars = " ".join([f"-D{key}={value}" for key, value in compile_time_vars])

        if self.tofino_version == 1:
            make_target = "install"
        elif self.tofino_version == 2:
            make_target = "install-tofino2"
        else:
            self.host.crash(f"We are only compatible with Tofino 1 and 2.")

        env_vars = " ".join(
            [
                f"APP={program_name}",
                f"SDE={self.sde}",
                f"SDE_INSTALL={self.sde}/install",
                f'P4_COMPILATION_VARS="{compilation_vars}"',
                f'CONTROLLER_ARGS=""',  # hack to ignore controller args
            ]
        )

        compilation_cmd = f"{env_vars} make -f {self.makefile} {make_target}"
        cmd = self.host.run_command(compilation_cmd, dir=src_path.parent)
        cmd.watch()
        code = cmd.recv_exit_status()

        if code != 0:
            self.host.crash(f"P4 compilation failed (app={program_name}).")

    def launch(self, app_name: str) -> None:
        if self.ready:
            return

        if self.host.is_proc_running("bf_switchd"):
            self.kill_switchd()

        env_vars = " ".join(
            [
                f"SDE={self.sde}",
                f"SDE_INSTALL={self.sde}/install",
            ]
        )

        cmd = f"{env_vars} ./run_switchd.sh -p {app_name}"

        if self.tofino_version == 2:
            cmd += " --arch tf2"

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
