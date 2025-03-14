import re
import itertools
import enum

from paramiko import ssh_exception

from pathlib import Path
from typing import Optional, Union

from .remote import RemoteHost
from .dpdk_config import DpdkConfig

MIN_NUM_TX_CORES = 2
PKTGEN_PROMPT = "Pktgen> "


class TrafficDist(enum.Enum):
    UNIFORM = "uniform"
    ZIPF = "zipf"


class Pktgen:
    def __init__(
        self,
        hostname: str,
        repo: str,
        rx_pcie_dev: str,
        tx_pcie_dev: str,
        nb_tx_cores: int,
        log_file: Optional[str] = None,
    ) -> None:
        self.host = RemoteHost(hostname, log_file=log_file)
        self.pktgen_dir = Path(repo) / "deps" / "pktgen"
        self.pktgen_exe = self.pktgen_dir / "Release" / "bin" / "pktgen"
        self.rx_pcie_dev = rx_pcie_dev
        self.tx_pcie_dev = tx_pcie_dev
        self.nb_tx_cores = nb_tx_cores

        self.setup_env_script = Path(repo) / "paths.sh"

        self.pktgen_active = False

        self.max_churn = 0
        self.ready = False

        if self.nb_tx_cores < MIN_NUM_TX_CORES:
            raise Exception(f"Number of TX cores must be >= {MIN_NUM_TX_CORES} (is {self.nb_tx_cores})")

        self.host.test_connection()

        if not self.host.remote_dir_exists(Path(repo)):
            self.host.crash(f"Repo not found on remote host {self.host}")

        self._build()

    def _build(self):
        build_script = self.pktgen_dir / "build.sh"

        cmd = self.host.run_command(f"source {self.setup_env_script} && {build_script}")
        cmd.watch()
        code = cmd.recv_exit_status()

        if code != 0:
            self.host.crash(f"Failed to build pktgen.")

        assert self.host.remote_file_exists(self.pktgen_exe)

    def _run_commands(self, cmds: Union[str, list[str]], wait_from_prompt: bool = True) -> str:
        assert self.pktgen_active
        return self.pktgen.run_console_commands(
            cmds,
            console_pattern=PKTGEN_PROMPT if wait_from_prompt else None,
        )

    def kill_pktgen(self) -> None:
        self.host.run_command(f"sudo killall -s SIGKILL {self.pktgen_exe.name}")
        self.pktgen_active = False
        self.ready = False

    def launch(
        self,
        nb_flows: int = 10_000,
        traffic_dist: TrafficDist = TrafficDist.UNIFORM,
        zipf_param: float = 1.26,
        pkt_size: int = 64,
        exp_time_us: int = 0,
        crc_unique_flows: bool = False,
        crc_bits: int = 16,
        seed: int = 0,
        mark_warmup_packets: bool = False,
        kvs_mode: bool = False,
    ) -> None:
        assert not self.pktgen_active

        self.kill_pktgen()

        # This is kind of lazy, and not sure if even correct, but let's
        # grab the last digit from he PCIe ID and use it as the port ID.
        tx_port = int(self.tx_pcie_dev.split(".")[-1])
        rx_port = int(self.rx_pcie_dev.split(".")[-1])

        pktgen_options_list = [
            f"--total-flows {nb_flows}",
            f"--dist {traffic_dist.value}",
            f"--zipf-param {zipf_param}",
            f"--pkt-size {pkt_size}",
            f"--tx {tx_port}",
            f"--rx {rx_port}",
            f"--tx-cores {self.nb_tx_cores}",
            f"--exp-time {exp_time_us}",
            f"--seed {seed}",
        ]

        if crc_unique_flows:
            pktgen_options_list.append(f"--crc-unique-flows")
            pktgen_options_list.append(f"--crc-bits {crc_bits}")

        if mark_warmup_packets:
            pktgen_options_list.append(f"--mark-warmup-packets")

        if kvs_mode:
            pktgen_options_list.append(f"--kvs-mode")

        pktgen_options = " ".join(pktgen_options_list)

        remote_cmd = f"source {self.setup_env_script} && sudo -E {str(self.pktgen_exe)} {self.dpdk_config} -- {pktgen_options}"

        self.pktgen = self.host.run_command(remote_cmd, pty=True)
        self.pktgen_active = True
        self.ready = False

        self.remote_cmd = remote_cmd
        self.target_pkt_tx = 0

    def wait_launch(self) -> int:
        assert self.pktgen_active

        if self.ready:
            return self.max_churn

        output = self.wait_ready()

        self.ready = True
        lines = output.split("\r\n")

        while lines:
            # Look for the header:
            # ----- Config -----
            if "Config" in lines[0]:
                break
            lines = lines[1:]

        for line in lines:
            match = re.search(r"\s*Max churn:\s*(\d+) fpm", line)

            if not match:
                continue

            self.max_churn = int(match.group(1))
            return self.max_churn

        raise Exception("Unable to retrieve max churn data from output")

    def wait_ready(self) -> str:
        assert self.pktgen_active

        # Wait to see if we actually managed to run pktgen successfuly.
        # Typically we fail here if we forgot to bind ports to DPDK,
        # or allocate hugepages.
        if self.pktgen.exit_status_ready() and self.pktgen.recv_exit_status() != 0:
            self.pktgen_active = False
            output = self.pktgen.watch()
            self.host.log(output)
            raise Exception("Cannot run pktgen")

        return self.pktgen.watch(stop_pattern=PKTGEN_PROMPT)

    def start(self) -> None:
        assert self.pktgen_active

        if not self.ready:
            self.wait_launch()

        self._run_commands("start")

    def set_warmup_duration(self, duration_sec: int) -> None:
        assert self.pktgen_active

        if not self.ready:
            self.wait_launch()

        self._run_commands(f"warmup {duration_sec}")

    def run(self, duration_sec: int) -> None:
        assert self.pktgen_active

        if not self.ready:
            self.wait_launch()

        self._run_commands(f"run {duration_sec}")

    def set_rate(self, rate_mbps: int) -> None:
        assert rate_mbps > 0
        self._run_commands(f"rate {rate_mbps}")

    def set_churn(self, churn_fpm: int) -> None:
        assert churn_fpm >= 0
        self._run_commands(f"churn {churn_fpm}")

    def stop(self) -> None:
        assert self.pktgen_active
        self._run_commands("stop")

    def reset_stats(self) -> None:
        assert self.pktgen_active
        self._run_commands("reset")

    def close(self) -> None:
        assert self.pktgen_active
        self._run_commands("quit", wait_from_prompt=False)
        self.pktgen_active = False
        self.host.log("Pktgen exited successfuly.")

    def get_stats(self) -> tuple[int, int, int, int]:
        assert self.pktgen_active
        output = self._run_commands("stats")

        lines = output.split("\r\n")

        while lines:
            if lines[0] == "~~~~~~ Pktgen ~~~~~~":
                break
            lines = lines[1:]

        assert lines

        tx_line = lines[1]
        rx_line = lines[2]

        tx_result = re.search(r"\s+TX:\s+(\d+) pkts (\d+) bytes", tx_line)
        rx_result = re.search(r"\s+RX:\s+(\d+) pkts (\d+) bytes", rx_line)

        assert tx_result
        assert rx_result

        tx_pkts = int(tx_result.group(1))
        tx_bytes = int(tx_result.group(2))
        rx_pkts = int(rx_result.group(1))
        rx_bytes = int(rx_result.group(2))

        return tx_pkts, tx_bytes, rx_pkts, rx_bytes

    def enter_interactive(self) -> None:
        self.pktgen.posix_shell()

    @property
    def dpdk_config(self):
        if hasattr(self, "_dpdk_config"):
            return self._dpdk_config

        self.host.validate_pcie_dev(self.rx_pcie_dev)
        self.host.validate_pcie_dev(self.tx_pcie_dev)

        all_cores = self.host.get_all_cpus()

        all_cores = set(self.host.get_pcie_dev_cpus(self.tx_pcie_dev))

        # Needs an extra core for the main thread.
        if len(all_cores) < self.nb_tx_cores + 1:
            raise Exception(f"Cannot find {self.nb_tx_cores + 1} cores")

        tx_cores = list(itertools.islice(all_cores, self.nb_tx_cores + 1))

        dpdk_config = DpdkConfig(
            cores=tx_cores,
            proc_type="auto",
            pci_allow_list=[self.rx_pcie_dev, self.tx_pcie_dev],
        )

        self._dpdk_config = dpdk_config
        return self._dpdk_config

    def __del__(self) -> None:
        try:
            if self.pktgen_active:
                self.close()
        except OSError:
            # Not important if we crash here.
            pass
        except ssh_exception.SSHException:
            # Not important if we crash here.
            pass
