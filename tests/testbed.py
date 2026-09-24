#!/usr/bin/env python3
"""
Lifecycle of the Tofino 2 model testbed: a P4 program and its controller.

Runs inside the SDE container (needs $SDE and $SDE_INSTALL) as root:

    sudo -E ./testbed.py up --p4 ../synthesized/nat-f40000-c0-unif-hmax-tput.p4 \
                           --controller ../synthesized/nat-f40000-c0-unif-hmax-tput.cpp
    sudo -E ./testbed.py up --p4 ../tofino/meta4/p4/meta4.p4 --controller ../tofino/meta4/meta4.py
    sudo -E ./testbed.py status
    sudo -E ./testbed.py down

The program's name is the P4 file's stem. A .cpp controller is a Barefoot program that starts
bf_switchd itself (a synthesized one does so through libsycon); a .py controller is a script run
against a bf_switchd started here, which configures the switch and exits.

The NF test scripts (echo.py, ...) assume the testbed is already up unless
they are given --up, in which case they call into this module themselves.
"""

from __future__ import annotations  # keep 3.9+ annotation syntax valid on Python 3.8

import os
import re
import subprocess
import sys
import time
from argparse import ArgumentParser
from pathlib import Path
from subprocess import PIPE, STDOUT
from typing import Callable, Optional

TESTS_DIR = Path(__file__).resolve().parent
PROJECT_DIR = TESTS_DIR.parent
TOFINO_TOOLS_DIR = PROJECT_DIR / "tofino" / "tools"

MAKEFILE = TOFINO_TOOLS_DIR / "Makefile"
PORTS_FILE = TOFINO_TOOLS_DIR / "ports_tof2.json"
VETH_SETUP_SCRIPT = TOFINO_TOOLS_DIR / "veth_setup.sh"

LOG_ROOT = Path("/tmp/synapse-tests")

# Front panel ports handed to the controller (--ports). The synthesized
# controllers only wire ports 3..32 to NF devices (see configs/tofino2.toml),
# but the controller is always launched with all of them.
FRONT_PANEL_PORTS = list(range(1, 33))

MODEL_READY_TIMEOUT_SEC = 30
CONTROLLER_READY_TIMEOUT_SEC = 180
STOP_TIMEOUT_SEC = 20

CONTROLLER_READY_MSG = "Controller is running"


class TestbedError(Exception):
    pass


def log(msg: str) -> None:
    print(f"[testbed] {msg}", flush=True)


def require_root() -> None:
    if os.geteuid() != 0:
        raise TestbedError("must run as root (sudo -E)")


def sde_env() -> dict[str, str]:
    env = dict(os.environ)
    for var in ("SDE", "SDE_INSTALL"):
        if var not in env:
            raise TestbedError(f"{var} env var not set (run inside the SDE container with sudo -E)")
    return env


def nf_log_dir(nf: str) -> Path:
    d = LOG_ROOT / nf
    d.mkdir(parents=True, exist_ok=True)
    return d


def model_log_dir(nf: str) -> Path:
    d = nf_log_dir(nf) / "model"
    d.mkdir(parents=True, exist_ok=True)
    return d


def controller_log_file(nf: str) -> Path:
    return nf_log_dir(nf) / "controller.log"


def build_log_file(nf: str) -> Path:
    return nf_log_dir(nf) / "build.log"


def controller_binary(controller: Path) -> Path:
    """Where the Makefile puts the debug build of a .cpp controller."""
    return controller.parent / "build" / "debug" / controller.stem


def controller_is_script(controller: Path) -> bool:
    if controller.suffix == ".py":
        return True
    if controller.suffix == ".cpp":
        return False
    raise TestbedError(f"controller must be a .cpp program or a .py script: {controller}")


def switchd_log_file(nf: str) -> Path:
    return nf_log_dir(nf) / "switchd.log"


def sde_python_path(env: dict[str, str]) -> str:
    """The SDE's python packages, as controller scripts expect them on PYTHONPATH."""
    for site_packages in Path(env["SDE_INSTALL"]).glob("lib/python*/site-packages"):
        if (site_packages / "tofino").is_dir():
            return f"{site_packages / 'tofino'}:{site_packages}"
    raise TestbedError(f"no tofino python packages under {env['SDE_INSTALL']}")


def tail(path: Path, lines: int = 30) -> str:
    try:
        return "".join(path.read_text(errors="replace").splitlines(keepends=True)[-lines:])
    except FileNotFoundError:
        return ""


def _is_zombie(pid: int) -> bool:
    """A process that has exited but was never reaped. It holds no port and no memory, yet it keeps
    its name, so pgrep still reports it: taken for a live model it makes `running_model` answer "?"
    forever and every start time out, and no signal can clear it (only its parent reaping it can)."""
    try:
        stat = Path(f"/proc/{pid}/stat").read_text()
    except OSError:
        return False
    # The state letter follows the parenthesised command name, which may itself contain spaces.
    _, _, after_name = stat.rpartition(") ")
    return after_name.startswith("Z")


def _pgrep(pattern: str) -> list[tuple[int, str]]:
    proc = subprocess.run(["pgrep", "-a", "-f", pattern], stdout=PIPE, stderr=STDOUT, text=True)
    result = []
    for line in proc.stdout.splitlines():
        pid, _, cmd = line.partition(" ")
        if pid.isdigit() and int(pid) != os.getpid() and not _is_zombie(int(pid)):
            result.append((int(pid), cmd))
    return result


def running_model() -> Optional[str]:
    """Name of the P4 program loaded by a running tofino-model, if any."""
    for _, cmd in _pgrep(r"^tofino-model\b"):
        m = re.search(r"--p4-target-config\s+\S*/([^/\s]+)\.conf", cmd)
        return m.group(1) if m else "?"
    return None


def _controller_procs() -> list[tuple[int, str]]:
    """Processes whose executable is a built controller (not shells/sudo wrapping one)."""
    build_dir = "/build/debug/"
    return [(pid, cmd) for pid, cmd in _pgrep(build_dir) if build_dir in cmd.split(" ")[0]]


def running_controller() -> Optional[str]:
    """Name of the running .cpp controller, if any."""
    for _, cmd in _controller_procs():
        return Path(cmd.split(" ")[0]).name
    return None


def running_switchd() -> Optional[str]:
    """Name of the P4 program a running bf_switchd (started for a controller script) serves, if any."""
    for _, cmd in _pgrep(r"\bbf_switchd\b"):
        m = re.search(r"--conf-file\s+\S*/([^/\s]+)\.conf", cmd)
        return m.group(1) if m else "?"
    return None


def ensure_veths() -> None:
    if Path("/sys/class/net/veth0").exists() and Path("/sys/class/net/veth251").exists():
        return
    log("setting up veth interfaces")
    subprocess.run([str(VETH_SETUP_SCRIPT)], check=True, stdout=PIPE, stderr=STDOUT)


def build(p4: Path, controller: Path) -> None:
    """Compile+install the P4 program and, for a .cpp controller, build it (make skips what is up to date)."""
    env = sde_env()
    logfile = build_log_file(p4.stem)

    for f in (p4, controller):
        if not f.is_file():
            raise TestbedError(f"missing {f}")

    # The P4 install is left incremental (bf-p4c takes minutes and the .p4 does not depend on
    # libsycon). The controller is always rebuilt (-B): it is a single g++ of one .cpp (seconds) and
    # it is the only half that depends on libsycon, whose headers make does not track. Always
    # recompiling it avoids stale binaries linking against an out-of-date libsycon (ABI mismatch).
    # Either is built by tofino/tools/Makefile in the source's own directory, APP naming the file.
    steps = [("install-tofino2", [], p4)]
    if not controller_is_script(controller):
        steps.append(("controller-debug", ["-B"], controller))
    with open(logfile, "w") as out:
        for target, extra, source in steps:
            env["APP"] = source.stem
            log(f"make {target} ({source.name})")
            proc = subprocess.run(
                ["make", *extra, "-f", str(MAKEFILE), target],
                cwd=source.parent,
                env=env,
                stdout=out,
                stderr=STDOUT,
            )
            if proc.returncode != 0:
                raise TestbedError(f"make {target} failed for {source.name}; see {logfile}\n{tail(logfile)}")


def _wait_for(predicate, timeout_sec: float, what: str, failed=lambda: False) -> None:
    deadline = time.monotonic() + timeout_sec
    while time.monotonic() < deadline:
        if predicate():
            return
        if failed():
            raise TestbedError(f"{what}: process died")
        time.sleep(0.25)
    raise TestbedError(f"{what}: timed out after {timeout_sec}s")


def start_model(nf: str) -> None:
    env = sde_env()
    logdir = model_log_dir(nf)
    stdout = logdir / "stdout.log"

    log(f"starting tofino-model for {nf} (logs: {logdir})")
    with open(stdout, "w") as out:
        proc = subprocess.Popen(
            [
                f"{env['SDE']}/run_tofino_model.sh",
                "-p", nf,
                "--arch", "tf2",
                "-f", str(PORTS_FILE),
                "--log-dir", str(logdir),
            ],
            cwd=logdir,
            env=env,
            stdout=out,
            stderr=STDOUT,
            stdin=subprocess.DEVNULL,
            start_new_session=True,
        )

    def ready() -> bool:
        return running_model() == nf and f"Loaded {PORTS_FILE}" in tail(stdout, 200)

    _wait_for(ready, MODEL_READY_TIMEOUT_SEC, "tofino-model start", failed=lambda: proc.poll() is not None)
    log("tofino-model is up")


def start_controller(controller: Path, nf: str) -> None:
    env = sde_env()
    binary = controller_binary(controller)
    if not binary.is_file():
        raise TestbedError(f"controller binary not found: {binary} (build it first)")

    logfile = controller_log_file(nf)
    log(f"starting controller {binary.name} (log: {logfile})")

    with open(logfile, "w") as out:
        proc = subprocess.Popen(
            [str(binary), "--ports", *[str(p) for p in FRONT_PANEL_PORTS], "--model"],
            cwd=nf_log_dir(nf),
            env=env,
            stdout=out,
            stderr=STDOUT,
            stdin=subprocess.DEVNULL,
            start_new_session=True,
        )

    def ready() -> bool:
        return CONTROLLER_READY_MSG in tail(logfile, 50)

    try:
        _wait_for(ready, CONTROLLER_READY_TIMEOUT_SEC, "controller start", failed=lambda: proc.poll() is not None)
    except TestbedError as e:
        raise TestbedError(f"{e}\n--- {logfile} ---\n{tail(logfile)}")
    log("controller is running")


# bf_switchd's stdin is bfshell's; it must stay open or switchd exits, so the pipe lives here.
_switchd_stdin = None


def start_switchd(nf: str) -> None:
    global _switchd_stdin
    env = sde_env()
    logfile = switchd_log_file(nf)

    log(f"starting bf_switchd for {nf} (log: {logfile})")
    with open(logfile, "w") as out:
        proc = subprocess.Popen(
            [f"{env['SDE']}/run_switchd.sh", "-p", nf, "--arch", "tf2"],
            cwd=nf_log_dir(nf),
            env=env,
            stdout=out,
            stderr=STDOUT,
            stdin=subprocess.PIPE,
            start_new_session=True,
        )
    _switchd_stdin = proc.stdin

    def ready() -> bool:
        return "bf_switchd: server started" in tail(logfile, 200)

    try:
        _wait_for(ready, CONTROLLER_READY_TIMEOUT_SEC, "bf_switchd start", failed=lambda: proc.poll() is not None)
    except TestbedError as e:
        raise TestbedError(f"{e}\n--- {logfile} ---\n{tail(logfile)}")
    log("bf_switchd is up")


def run_controller_script(script: Path, nf: str) -> None:
    """Runs a controller script to completion: it configures the switch and exits."""
    env = sde_env()
    env["PYTHONPATH"] = sde_python_path(env)
    if not script.is_file():
        raise TestbedError(f"controller not found: {script}")

    logfile = controller_log_file(nf)
    log(f"running controller {script.name} (log: {logfile})")

    # bfrt's gRPC server comes up a little after switchd reports itself started.
    attempts = 6
    for attempt in range(1, attempts + 1):
        with open(logfile, "a") as out:
            proc = subprocess.run([str(script)], cwd=script.parent, env=env, stdout=out, stderr=STDOUT)
        if proc.returncode == 0:
            log("controller done")
            return
        if attempt < attempts:
            time.sleep(5)
    raise TestbedError(f"{script.name} failed {attempts} times; see {logfile}\n{tail(logfile)}")


def _kill(find: Callable[[], list[tuple[int, str]]], what: str) -> None:
    procs = find()
    if not procs:
        return
    pids = [pid for pid, _ in procs]
    log(f"stopping {what} (pids {pids})")
    subprocess.run(["kill", "-TERM", *map(str, pids)], stdout=PIPE, stderr=STDOUT)
    deadline = time.monotonic() + STOP_TIMEOUT_SEC
    while time.monotonic() < deadline and find():
        time.sleep(0.25)
    left = find()
    if left:
        log(f"{what} did not exit, killing")
        subprocess.run(["kill", "-KILL", *[str(pid) for pid, _ in left]], stdout=PIPE, stderr=STDOUT)
        time.sleep(0.5)


def down() -> None:
    global _switchd_stdin
    # Controller first (it is the bf_switchd side), then the model.
    _kill(_controller_procs, "controller")
    _kill(lambda: _pgrep(r"\bbf_switchd\b"), "bf_switchd")
    _kill(lambda: _pgrep(r"run_switchd\.sh"), "run_switchd.sh")
    _switchd_stdin = None
    _kill(lambda: _pgrep(r"^tofino-model\b"), "tofino-model")
    _kill(lambda: _pgrep(r"run_tofino_model\.sh"), "run_tofino_model.sh")


def up(p4: Path, controller: Path, do_build: bool = True) -> None:
    require_root()
    ensure_veths()
    nf = p4.stem
    script = controller_is_script(controller)
    if do_build:
        build(p4, controller)
    if running_model() or running_controller() or running_switchd():
        down()
    start_model(nf)
    try:
        if script:
            start_switchd(nf)
            run_controller_script(controller, nf)
        else:
            start_controller(controller, nf)
    except TestbedError:
        down()
        raise


def status() -> None:
    model = running_model()
    controller = running_controller()
    switchd = running_switchd()
    print(f"tofino-model: {'running (' + model + ')' if model else 'not running'}")
    print(f"controller:   {'running (' + controller + ')' if controller else 'not running'}")
    print(f"bf_switchd:   {'running (' + switchd + ')' if switchd else 'not running'}")


def assert_up(p4: Path, controller: Path) -> None:
    """Fail unless the testbed is running for exactly this program and controller."""
    nf = p4.stem
    model = running_model()
    if controller_is_script(controller):
        side, expected = running_switchd(), nf
    else:
        side, expected = running_controller(), controller.stem
    if side != expected or model != nf:
        raise TestbedError(
            f"testbed is not up for {nf} (model: {model}, {'bf_switchd' if controller_is_script(controller) else 'controller'}: {side}); "
            f"run `sudo -E {TESTS_DIR / 'testbed.py'} up --p4 {p4} --controller {controller}` or pass --up"
        )


def main() -> int:
    parser = ArgumentParser(description=__doc__.strip().splitlines()[0])
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_up = sub.add_parser("up", help="build (unless --no-build), then start model + controller")
    p_up.add_argument("--p4", type=Path, required=True, help="P4 program to compile and install; its stem names the program")
    p_up.add_argument("--controller", type=Path, required=True, help=".cpp: a program that starts bf_switchd itself; .py: a script run against a bf_switchd started here")
    p_up.add_argument("--no-build", action="store_true")

    p_build = sub.add_parser("build", help="build P4 + controller only")
    p_build.add_argument("--p4", type=Path, required=True)
    p_build.add_argument("--controller", type=Path, required=True)

    sub.add_parser("down", help="stop controller and model")
    sub.add_parser("status")

    args = parser.parse_args()

    try:
        if args.cmd == "up":
            up(args.p4.resolve(), args.controller.resolve(), do_build=not args.no_build)
        elif args.cmd == "build":
            build(args.p4.resolve(), args.controller.resolve())
        elif args.cmd == "down":
            require_root()
            down()
        elif args.cmd == "status":
            status()
    except TestbedError as e:
        print(f"[testbed] ERROR: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
