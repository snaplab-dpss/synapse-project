import itertools

from paramiko import ssh_exception

from pathlib import Path
from typing import Optional

from .remote import RemoteHost
from .dpdk_config import DpdkConfig


class KVSServer:
    def __init__(
        self,
        hostname: str,
        repo: str,
        pcie_dev: str,
        nb_cores: int = 16,
        log_file: Optional[str] = None,
    ) -> None:
        self.host = RemoteHost(hostname, log_file=log_file)
        self.server_dir = Path(repo) / "key_value_store"
        self.server_exe = self.server_dir / "build" / "release" / "server"
        self.pcie_dev = pcie_dev
        self.nb_cores = nb_cores

        self.setup_env_script = Path(repo) / "paths.sh"

        self.server_active = False
        self.ready = False

        self.host.test_connection()

        if not self.host.remote_dir_exists(Path(repo)):
            self.host.crash(f"Repo not found on remote host {self.host}")

        self._build()

    def _build(self):
        build_script = self.server_dir / "build.sh"

        cmd = self.host.run_command(f"source {self.setup_env_script} && {build_script}")
        cmd.watch()
        code = cmd.recv_exit_status()

        if code != 0:
            self.host.crash(f"Failed to build KVS server.")

        assert self.host.remote_file_exists(self.server_exe)

    def kill_server(self) -> None:
        self.host.run_command(f"sudo killall {self.server_exe.name}")
        self.server_active = False
        self.ready = False

    def launch(
        self,
        delay_ns: int,
    ) -> None:
        assert not self.server_active

        self.kill_server()

        args = f"--delay {delay_ns}"
        remote_cmd = f"source {self.setup_env_script} && sudo -E {str(self.server_exe)} {self.dpdk_config} -- {args}"

        self.server = self.host.run_command(remote_cmd, pty=True)
        self.server_active = True
        self.ready = False

    def wait_launch(self) -> None:
        assert self.server_active

        if self.ready:
            return

        self._wait_ready()
        self.ready = True

    def _wait_ready(self) -> str:
        assert self.server_active

        # Wait to see if we actually managed to run pktgen successfuly.
        # Typically we fail here if we forgot to bind ports to DPDK,
        # or allocate hugepages.
        if self.server.exit_status_ready():
            self.server_active = False
            output = self.server.watch()
            self.host.log(output)
            self.host.crash("Failed to run KVS server.")

        output = self.server.watch(stop_pattern="All lcores ready")

        if self.server.exit_status_ready():
            self.server_active = False
            self.host.log(output)
            self.host.crash("Failed to run KVS server.")

        return output

    @property
    def dpdk_config(self):
        if hasattr(self, "_dpdk_config"):
            return self._dpdk_config

        self.host.validate_pcie_dev(self.pcie_dev)

        all_cores = self.host.get_all_cpus()
        all_cores = set(self.host.get_pcie_dev_cpus(self.pcie_dev))

        cores = list(itertools.islice(all_cores, self.nb_cores))

        dpdk_config = DpdkConfig(
            cores=cores,
            proc_type="auto",
            pci_allow_list=[self.pcie_dev],
        )

        self._dpdk_config = dpdk_config
        return self._dpdk_config

    def __del__(self) -> None:
        try:
            if self.server_active:
                self.kill_server()
        except OSError:
            # Not important if we crash here.
            pass
        except ssh_exception.SSHException:
            # Not important if we crash here.
            pass
