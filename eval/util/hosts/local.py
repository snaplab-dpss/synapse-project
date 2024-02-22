from .commands.command import Command
from .commands.local import LocalCommand

from .host import Host

from typing import Optional

class LocalHost(Host):
    def __init__(
        self,
        log_file: Optional[str] = None,
    ) -> None:
        super().__init__(log_file)

    def run_command(self, *args, **kwargs) -> Command:
        return LocalCommand(*args, **kwargs)
