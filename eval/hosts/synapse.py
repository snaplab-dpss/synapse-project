#!/usr/bin/env python3

from paramiko import ssh_exception

from pathlib import Path
from typing import Optional, Union

import re

from .remote import RemoteHost

MIN_TIMEOUT = 10  # miliseconds
SYNAPSE_BENCH_CONTROLLER_PROMPT = "Sycon> "


class SynapseController:
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
        self.controller_cmd = None
        self.exe = None

        self.host.test_connection()

        if not self.host.remote_dir_exists(self.repo):
            self.host.crash(f"Repo not found on remote host {self.host}")

        if not self.host.remote_dir_exists(self.sde):
            self.host.crash(f"SDE directory not found on remote host {self.sde}")

        self.ready = False

    def _clean(self, src: Path) -> None:
        if not self.host.remote_file_exists(src):
            self.host.crash(f"Unable to find file {src}")

        makefile = self.repo / "tofino" / "tools" / "Makefile"

        env_vars = " ".join(
            [
                f"SDE={self.sde}",
                f"SDE_INSTALL={self.sde}/install",
            ]
        )

        compilation_cmd = f"{env_vars} APP={src.stem} make -f {makefile} clean"
        cmd = self.host.run_command(compilation_cmd, dir=src.parent)
        cmd.watch()
        code = cmd.recv_exit_status()

        if code != 0:
            self.host.crash(f"Controller cleanup failed.")

    def _compile(self, src: Path) -> None:
        # Cleanup first, always recompile from scratch.
        self._clean(src)

        if not self.host.remote_file_exists(src):
            self.host.crash(f"Unable to find file {src}")

        makefile = self.repo / "tofino" / "tools" / "Makefile"

        env_vars = " ".join(
            [
                f"SDE={self.sde}",
                f"SDE_INSTALL={self.sde}/install",
            ]
        )

        compilation_cmd = f"{env_vars} APP={src.stem} make -f {makefile} controller -j"
        cmd = self.host.run_command(compilation_cmd, dir=src.parent)
        cmd.watch()
        code = cmd.recv_exit_status()

        if code != 0:
            self.host.crash(f"Controller compilation failed (app={src.stem}).")

    def launch(
        self,
        src_in_repo: str,
        ports: list[int],
        timeout_ms: int = MIN_TIMEOUT,
        extra_args: list[tuple[str, Union[str, int, float]]] = [],
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
        cmd += f" --bench"
        cmd += f" --ports {' '.join(map(str, ports))}"
        cmd += f" --expiration-time {timeout_ms}"

        for extra_arg in extra_args:
            cmd += f" {extra_arg[0]} {extra_arg[1]}"

        self.controller_cmd = self.host.run_command(cmd, dir=src_path.parent)
        self.ready = False

    def wait_ready(self) -> None:
        # Wait for the controller to be ready.
        # It prints the message "Press enter to terminate." when it's ready.

        if self.ready:
            # We already checked that it's ready
            return

        if self.controller_cmd is None:
            raise RuntimeError("Controller not started")

        self.controller_cmd.watch(
            stop_pattern=SYNAPSE_BENCH_CONTROLLER_PROMPT,
        )

        if self.controller_cmd.exit_status_ready():
            self.host.crash("Controller exited unexpectedly")

        self.ready = True

    def send_usr1_signal(self) -> Optional[str]:
        if self.controller_cmd is None:
            return None

        cmd = f"sudo killall -SIGUSR1 {self.exe}"

        # Flush stdout and stderr before running the new command
        self.controller_cmd.flush()

        output = ""
        while len(output) == 0:
            self.host.run_command(cmd)
            output = self.controller_cmd.watch(
                stop_pattern="~~~ USR1 signal processing done ~~~",
                timeout=5,
            )

        return output

    def get_port_stats(self) -> dict[int, tuple[int, int]]:
        assert self.ready
        assert self.controller_cmd

        output = self.controller_cmd.run_console_commands(
            commands=["stats"],
            console_pattern=SYNAPSE_BENCH_CONTROLLER_PROMPT,
        )

        output_split = output.rstrip().split(" ")
        assert output_split[0] == "STATS"

        stats = {}
        for entry in output_split[1:]:
            if "\n" in entry:
                break

            entry_split = entry.split(":")
            assert len(entry_split) == 3
            port = int(entry_split[0])
            rx = int(entry_split[1])
            tx = int(entry_split[2])
            stats[port] = (rx, tx)

        return stats

    def reset_port_stats(self) -> None:
        assert self.ready
        assert self.controller_cmd

        self.controller_cmd.run_console_commands(
            commands=["reset"],
            console_pattern=SYNAPSE_BENCH_CONTROLLER_PROMPT,
        )

    def quit(self) -> None:
        assert self.ready
        assert self.controller_cmd

        self.controller_cmd.run_console_commands(
            commands=["quit"],
            console_pattern=SYNAPSE_BENCH_CONTROLLER_PROMPT,
        )

        self.ready = False
        self.controller_cmd = None

    def get_cpu_counters(self) -> Optional[tuple[int, int, int]]:
        output = self.send_usr1_signal()

        if not output:
            return None

        result = re.search(r".*In:\s+(\d+)\nCPU:\s+(\d+)\nTotal:\s+(\d+).*", output)

        if not result:
            self.host.crash("Asked for CPU counters but information not found on controller output")

        assert result

        in_pkts = int(result.group(1))
        cpu_pkts = int(result.group(2))
        total_pkts = int(result.group(3))

        return in_pkts, cpu_pkts, total_pkts

    def stop(self):
        if self.controller_cmd is None:
            return

        self.host.run_command(f'sudo pkill -f "synapse"').watch()

        if self.exe:
            self.host.run_command(f"sudo killall {self.exe}").watch()

        self.host.log("Controller exited successfuly.")

        self.controller_cmd = None
        self.exe = None
        self.ready = False

    def __del__(self):
        try:
            self.stop()
        except ssh_exception.SSHException:
            # The ssh session is closed.
            pass
