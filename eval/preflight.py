#!/usr/bin/env python3
"""Preflight readiness check for the experiment testbed.

Verifies that every machine is configured to run the experiments *before* any
experiment is launched, so failures show up here instead of halfway through a
run. It checks the OS-level requirements set up by tools/remote_machines/setup_*.sh
(hugepages, DPDK bindings, loaded Barefoot drivers, BMC management interfaces),
and then brings up the traffic generator to report whether the required switch
ports are enabled, linked, and running at the expected speed.

Exit code is 0 only if every (non-skipped) check passes.
"""

import argparse
import re
import time
import tomli

from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

from rich.console import Console
from rich.table import Table

from hosts.remote import RemoteHost
from hosts.tofino_tg import TofinoTG, TofinoTGController, PortState
from hosts.kvs_server import KVSServer
from utils.constants import EVAL_DIR

# How long to give the switch ports to negotiate a link after switchd is up.
DEFAULT_PORT_TIMEOUT_S = 30

# Barefoot kernel modules that must be loaded for switchd to reach the ASIC.
# These are SDE-specific rather than testbed-specific, so they stay here.
TG_MODULES = ["bf_kdrv"]
DUT_MODULES = ["bf_kdrv", "bf_fpga"]


@dataclass
class CheckResult:
    name: str
    ok: bool
    detail: str = ""
    skipped: bool = False


@dataclass
class HostReport:
    alias: str
    title: str
    results: list[CheckResult] = field(default_factory=list)


def ssh(host: RemoteHost, cmd: str) -> tuple[int, str]:
    """Run a command on a remote host, returning (exit_code, stdout)."""
    command = host.run_command(cmd)
    output = command.watch()
    code = command.recv_exit_status()
    return code, output


# --------------------------------------------------------------------------- #
# Individual checks
# --------------------------------------------------------------------------- #
def check_reachable(host: RemoteHost, results: list[CheckResult]) -> bool:
    try:
        code, _ = ssh(host, "echo ok")
        if code == 0:
            return True
        results.append(CheckResult("reachable (ssh)", False, f"ssh exit {code}"))
    except Exception as e:
        results.append(CheckResult("reachable (ssh)", False, f"{type(e).__name__}: {e}"))
    return False


def check_dir(host: RemoteHost, path: str, name: str) -> CheckResult:
    code, _ = ssh(host, f"test -d {path}")
    return CheckResult(name, code == 0, "" if code == 0 else "not found")


def check_file(host: RemoteHost, path: str, name: str) -> CheckResult:
    code, _ = ssh(host, f"test -f {path}")
    return CheckResult(name, code == 0, "" if code == 0 else "not found")


def check_char_dev(host: RemoteHost, path: str, name: str) -> CheckResult:
    code, _ = ssh(host, f"test -e {path}")
    return CheckResult(name, code == 0, "" if code == 0 else f"{path} missing")


def check_module(host: RemoteHost, module: str) -> CheckResult:
    _, out = ssh(host, "lsmod")
    loaded = any(line.split() and line.split()[0] == module for line in out.splitlines())
    return CheckResult(f"kernel module '{module}' loaded", loaded, "" if loaded else "not loaded")


def check_iface_up(host: RemoteHost, iface: str, name: str) -> CheckResult:
    code, out = ssh(host, f"ip link show {iface}")
    if code != 0:
        return CheckResult(name, False, f"interface {iface} not found")
    up = bool(re.search(r"<[^>]*\bUP\b[^>]*>", out))
    return CheckResult(name, up, "" if up else f"{iface} is DOWN")


def check_hugepages(host: RemoteHost) -> CheckResult:
    # We only care that hugepages are available, not how many. This keeps the
    # check decoupled from whatever amount the setup scripts happen to reserve.
    name = "hugepages available"
    cmd = (
        "for d in /sys/kernel/mm/hugepages/hugepages-*kB; do "
        "echo \"$(basename \"$d\") $(cat \"$d/nr_hugepages\") $(cat \"$d/free_hugepages\")\"; "
        "done"
    )
    _, out = ssh(host, cmd)

    total_bytes = 0
    free_bytes = 0
    for line in out.splitlines():
        parts = line.split()
        if len(parts) != 3:
            continue
        size_match = re.search(r"(\d+)kB", parts[0])
        if not size_match:
            continue
        try:
            size_b = int(size_match.group(1)) * 1024
            total_bytes += size_b * int(parts[1])
            free_bytes += size_b * int(parts[2])
        except ValueError:
            continue

    total_gib = total_bytes / (1024 ** 3)
    free_gib = free_bytes / (1024 ** 3)
    ok = total_bytes > 0 and free_bytes > 0
    detail = f"{total_gib:.0f} GiB total, {free_gib:.0f} GiB free" if total_bytes > 0 else "none configured"
    return CheckResult(name, ok, detail)


def check_devbind(host: RemoteHost, devs: list[str], driver: str) -> CheckResult:
    name = f"DPDK devices bound to {driver}"
    code, out = ssh(host, "dpdk-devbind.py --status")
    if code != 0:
        return CheckResult(name, False, f"dpdk-devbind.py failed (exit {code})")

    lines = out.splitlines()
    not_bound = []
    for dev in devs:
        line = next((l for l in lines if dev in l), None)
        if line is None or f"drv={driver}" not in line:
            not_bound.append(dev)

    ok = not not_bound
    detail = f"{', '.join(devs)}" if ok else f"not bound: {', '.join(not_bound)}"
    return CheckResult(name, ok, detail)


# --------------------------------------------------------------------------- #
# Switch port capability check (requires bringing up the traffic generator)
# --------------------------------------------------------------------------- #
def speed_to_gbps(speed: str) -> Optional[int]:
    """Parse a switch speed string like 'BF_SPEED_100G' into Gbps (100)."""
    match = re.search(r"(\d+)G", speed or "")
    return int(match.group(1)) if match else None


def is_cabled(media_type: str) -> bool:
    """A cable/transceiver is present unless the media type is unknown/empty."""
    return bool(media_type) and "UNKNOWN" not in media_type


def port_verdict(st: Optional[PortState], expected_gbps: Optional[int]) -> tuple[bool, str]:
    """Return (ok, detail) for a single port's state.

    Uses $MEDIA_TYPE to distinguish "no cable/transceiver" from "cabled but the
    far end is down" -- the key hint for what to fix physically.
    """
    if st is None:
        return False, "not reported"
    if not st.enable:
        return False, "not enabled (admin down)"
    if not st.up:
        return False, "DOWN: no cable/transceiver" if not is_cabled(st.media_type) else "DOWN: cabled but far end down"
    gbps = speed_to_gbps(st.speed)
    if expected_gbps and gbps != expected_gbps:
        return False, f"speed {gbps}G, expected {expected_gbps}G"
    return True, (f"{gbps}G" if gbps is not None else "up")


def check_switch_ports(
    config: dict,
    port_timeout_s: int,
    expected_gbps: Optional[int],
    log_dir: str,
) -> list[CheckResult]:
    """Bring up the testbed and verify the three connection groups from config.

    A link only comes up when *both* ends are driving it, so we bring up every
    endpoint of each link: the traffic generator on BOTH switches (it builds for
    Tofino 1 and 2 and its `setup set` enables all front-panel ports) and the
    server's DPDK app (so the DUT<->server link can come up). Port state is then
    read from each switch over bfrt (traffic-generator.py), including $MEDIA_TYPE
    -- letting us tell "no cable/transceiver" apart from "cabled but far end down".

    The three groups, all derived from the config:
      (1) TG <-> DUT      : switch_tg.dut_ports  <-> switch_dut.client_ports
      (2) pktgen <-> TG   : switch_tg.pktgen_port
      (3) DUT <-> server  : switch_dut.server_port  (server DPDK app launched)
    """
    results: list[CheckResult] = []

    hosts = config["hosts"]
    repos = config["repo"]
    tg = config["devices"]["switch_tg"]
    dut = config["devices"]["switch_dut"]

    tg_dut_ports = list(tg["dut_ports"])
    pktgen_port = tg["pktgen_port"]
    client_ports = list(dut["client_ports"])
    server_port = dut["server_port"]

    tg_required = [pktgen_port] + tg_dut_ports
    dut_required = client_ports + [server_port]

    tg_switch: Optional[TofinoTG] = None
    dut_switch: Optional[TofinoTG] = None
    server: Optional[KVSServer] = None
    server_error: Optional[str] = None
    try:
        tg_switch = TofinoTG(hosts["switch_tg"], repos["switch_tg"], tg["sde"], tg["tofino_version"], log_file=f"{log_dir}/preflight_tg_switch.log")
        tg_controller = TofinoTGController(hosts["switch_tg"], repos["switch_tg"], tg["sde"], log_file=f"{log_dir}/preflight_tg_ctrl.log")
        dut_switch = TofinoTG(hosts["switch_dut"], repos["switch_dut"], dut["sde"], dut["tofino_version"], log_file=f"{log_dir}/preflight_dut_switch.log")
        dut_controller = TofinoTGController(hosts["switch_dut"], repos["switch_dut"], dut["sde"], log_file=f"{log_dir}/preflight_dut_ctrl.log")

        # Install + launch the traffic generator on both switches.
        tg_switch.install()
        dut_switch.install()
        tg_switch.launch()
        dut_switch.launch()
        tg_switch.wait_ready()
        dut_switch.wait_ready()

        # `setup set` enables every front-panel port on each switch (the broadcast
        # list only configures routing/multicast, which we do not care about here).
        tg_controller.setup(broadcast=tg_dut_ports, symmetric=[], route=[])
        dut_controller.setup(broadcast=client_ports, symmetric=[], route=[])

        # The DUT<->server link only comes up once the server's DPDK app drives its
        # NIC, so launch the server too -- otherwise the server port is a false DOWN.
        # A server-side failure must not mask groups (1)/(2), so isolate it.
        try:
            server = KVSServer(hosts["server"], repos["server"], config["devices"]["server"]["dev"], log_file=f"{log_dir}/preflight_server.log")
            server.launch(delay_ns=0)
            server.wait_launch()
        except SystemExit:
            server_error = f"could not start server (see {log_dir}/preflight_server.log)"
        except Exception as e:
            server_error = f"{type(e).__name__}: {e}"

        deadline = time.time() + port_timeout_s
        tg_state: dict[int, PortState] = {}
        dut_state: dict[int, PortState] = {}
        while True:
            tg_state = tg_controller.get_ports_state()
            dut_state = dut_controller.get_ports_state()
            tg_ok = all(p in tg_state and tg_state[p].up for p in tg_required)
            dut_ok = all(p in dut_state and dut_state[p].up for p in dut_required)
            if (tg_ok and dut_ok) or time.time() >= deadline:
                break
            time.sleep(3)

        # (1) TG <-> DUT: each TG dut_port pairs (index-aligned) with a DUT client_port.
        ready = 0
        pair_failures: list[CheckResult] = []
        for tg_port, dut_port in zip(tg_dut_ports, client_ports):
            tg_ok, tg_detail = port_verdict(tg_state.get(tg_port), expected_gbps)
            dut_ok, dut_detail = port_verdict(dut_state.get(dut_port), expected_gbps)
            if tg_ok and dut_ok:
                ready += 1
            else:
                pair_failures.append(CheckResult(f"TG {tg_port} <-> DUT {dut_port}", False, f"TG: {tg_detail}; DUT: {dut_detail}"))
        total = len(tg_dut_ports)
        results.append(CheckResult(f"(1) TG<->DUT links ({ready}/{total})", ready == total, "all up" if ready == total else "see failures below"))
        results.extend(pair_failures)

        # (2) pktgen <-> TG.
        ok, detail = port_verdict(tg_state.get(pktgen_port), expected_gbps)
        results.append(CheckResult(f"(2) pktgen<->TG (TG port {pktgen_port})", ok, detail))

        # (3) DUT <-> server (the server's DPDK app was launched above).
        if server_error:
            results.append(CheckResult(f"(3) DUT<->server (DUT port {server_port})", False, server_error))
        else:
            ok, detail = port_verdict(dut_state.get(server_port), expected_gbps)
            results.append(CheckResult(f"(3) DUT<->server (DUT port {server_port})", ok, detail))
    except SystemExit:
        # The host helpers call crash()->exit(1) on error; treat as a failed check.
        results.append(CheckResult("switch bring-up", False, f"switch/controller aborted (see {log_dir}/preflight_*.log)"))
    except Exception as e:
        results.append(CheckResult("switch bring-up", False, f"{type(e).__name__}: {e}"))
    finally:
        # Tear down the server app and both switches' bf_switchd.
        if server is not None:
            try:
                server.kill_server()
            except Exception:
                pass
        for switch in (tg_switch, dut_switch):
            if switch is not None:
                try:
                    switch.kill_switchd()
                except Exception:
                    pass

    return results


# --------------------------------------------------------------------------- #
# Orchestration
# --------------------------------------------------------------------------- #
def check_pktgen_host(alias, repo, dev) -> HostReport:
    report = HostReport(alias, f"pktgen  ({alias})")
    host = RemoteHost(alias)
    if not check_reachable(host, report.results):
        return report
    report.results.append(check_dir(host, repo, f"repo present ({repo})"))
    report.results.append(check_file(host, f"{repo}/paths.sh", "paths.sh present"))
    report.results.append(check_module(host, "uio"))
    report.results.append(check_module(host, "igb_uio"))
    report.results.append(check_devbind(host, [dev["tx_dev"], dev["rx_dev"]], "igb_uio"))
    report.results.append(check_hugepages(host))
    return report


def check_server_host(alias, repo, dev) -> HostReport:
    report = HostReport(alias, f"server  ({alias})")
    host = RemoteHost(alias)
    if not check_reachable(host, report.results):
        return report
    report.results.append(check_dir(host, repo, f"repo present ({repo})"))
    report.results.append(check_module(host, "uio"))
    report.results.append(check_module(host, "igb_uio"))
    report.results.append(check_devbind(host, [dev["dev"]], "igb_uio"))
    report.results.append(check_hugepages(host))
    return report


def check_switch_host(alias, role, repo, dev, modules) -> HostReport:
    report = HostReport(alias, f"{role}  ({alias})")
    host = RemoteHost(alias)
    if not check_reachable(host, report.results):
        return report
    report.results.append(check_dir(host, repo, f"repo present ({repo})"))
    report.results.append(check_dir(host, dev["sde"], f"SDE present ({dev['sde']})"))
    # The management/BMC interface name is device-specific, so it is optional and
    # read from the config. If it is not set, skip the check (portability).
    iface = dev.get("mgmt_iface")
    if iface:
        report.results.append(check_iface_up(host, iface, f"mgmt interface up ({iface})"))
    else:
        report.results.append(CheckResult("mgmt interface", True, "no mgmt_iface in config", skipped=True))
    for module in modules:
        report.results.append(check_module(host, module))
    report.results.append(check_char_dev(host, "/dev/bf0", "ASIC device node (/dev/bf0)"))
    return report


def render(console: Console, report: HostReport) -> bool:
    table = Table(title=report.title, title_style="bold", show_lines=False, expand=False)
    table.add_column("Check", style="cyan", no_wrap=False)
    table.add_column("Status", justify="center")
    table.add_column("Detail", style="dim")

    all_ok = True
    for r in report.results:
        if r.skipped:
            status = "[yellow]SKIP[/]"
        elif r.ok:
            status = "[green]PASS[/]"
        else:
            status = "[red]FAIL[/]"
            all_ok = False
        table.add_row(r.name, status, r.detail)

    console.print(table)
    console.print()
    return all_ok


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("-c", "--config-file", type=Path, default=EVAL_DIR / "experiment_config.toml", help="Path to config file")
    parser.add_argument("--skip-ports", action="store_true", help="Skip the (heavier) switch port capability check")
    parser.add_argument("--port-timeout", type=int, default=DEFAULT_PORT_TIMEOUT_S, help="Seconds to wait for switch ports to link")
    args = parser.parse_args()

    with open(args.config_file, "rb") as f:
        config = tomli.load(f)

    console = Console()
    console.rule("[bold]Testbed preflight check")

    hosts = config["hosts"]
    repos = config["repo"]
    devices = config["devices"]

    reports: list[HostReport] = [
        check_switch_host(hosts["switch_tg"], "TG switch", repos["switch_tg"], devices["switch_tg"], TG_MODULES),
        check_switch_host(hosts["switch_dut"], "DUT switch", repos["switch_dut"], devices["switch_dut"], DUT_MODULES),
        check_pktgen_host(hosts["pktgen"], repos["pktgen"], devices["pktgen"]),
        check_server_host(hosts["server"], repos["server"], devices["server"]),
    ]

    overall_ok = True
    for report in reports:
        overall_ok &= render(console, report)

    # The port check brings up both switches, so both need their base
    # requirements (repo, SDE, drivers, mgmt interface) satisfied first.
    tg_base_ok = all(r.ok for r in reports[0].results)
    dut_base_ok = all(r.ok for r in reports[1].results)

    port_report = HostReport(hosts["switch_tg"], "Switch ports (both switches up)")
    if args.skip_ports:
        port_report.results.append(CheckResult("switch ports", True, "skipped (--skip-ports)", skipped=True))
    elif not (tg_base_ok and dut_base_ok):
        port_report.results.append(CheckResult("switch ports", True, "skipped: fix TG/DUT switch base checks first", skipped=True))
    else:
        expected_gbps = devices["switch_tg"].get("port_speed") or None
        log_dir = str(EVAL_DIR / "logs")
        console.print("[dim]Bringing up both switches to probe port link state (this can take a few minutes)...[/]\n")
        port_report.results = check_switch_ports(config, args.port_timeout, expected_gbps, log_dir)

    overall_ok &= render(console, port_report)

    if overall_ok:
        console.print("[bold green]READY[/] — all checks passed. Safe to run experiments.")
        raise SystemExit(0)
    else:
        console.print("[bold red]NOT READY[/] — fix the FAIL items above before running experiments.")
        raise SystemExit(1)


if __name__ == "__main__":
    main()
