#!/usr/bin/env python3

from typing import Optional

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
