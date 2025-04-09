#!/usr/bin/env python3

from typing import Optional, Tuple

from .tofino_tg import TofinoTG, TofinoTGController

APP_NAME = "traffic-accelerator"


class TofinoTA(TofinoTG):
    def __init__(
        self,
        hostname: str,
        repo: str,
        sde: str,
        tofino_version: int,
        log_file: Optional[str] = None,
    ) -> None:
        super().__init__(hostname, repo, sde, tofino_version, log_file)
        self.app_name = APP_NAME


class TofinoTAController(TofinoTGController):
    def __init__(
        self,
        hostname: str,
        repo: str,
        sde: str,
        log_file: Optional[str] = None,
    ) -> None:
        super().__init__(hostname, repo, sde, log_file)
        self.app_name = APP_NAME
        self.multiplier = 1

    def setup(
        self,
        broadcast: list[int],
        symmetric: list[int],
        route: list[Tuple[int, int]],
        kvs_mode: bool,
    ) -> None:
        super().setup(broadcast, symmetric, route, kvs_mode)
        self.reset_to_default_acceleration()

    def reset_to_default_acceleration(self) -> None:
        self.set_acceleration(3)

    def set_acceleration(self, multiplier: int) -> None:
        self.multiplier = multiplier

        target_dir = self.repo / "tofino" / self.app_name
        cmd = f"{self.env_vars} ./{self.app_name}.py setup set-multiplier {self.multiplier}"

        command = self.host.run_command(cmd, dir=target_dir)
        command.watch()
        code = command.recv_exit_status()

        if code != 0:
            self.host.crash(f"{self.app_name} controller exited with code != 0.")
            exit(1)

    def get_pktgen_rate_for_true_rate(self, rate: int) -> int:
        pktgen_rate = int(rate / self.multiplier)
        if pktgen_rate == 0:
            self.host.log(f"WARNING: Requested rate is too low ({rate}) for the current multiplier ({self.multiplier}), setting to 1.")
            pktgen_rate = 1
        return pktgen_rate
