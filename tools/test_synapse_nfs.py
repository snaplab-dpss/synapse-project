#!/usr/bin/env python3
"""Run the model tests (tests/<nf>.py) over the synthesized solutions of every NF and workload.

Runs where the Tofino SDE is installed (needs $SDE and $SDE_INSTALL), as root, since each test
brings the Tofino 2 model and the solution's controller up on the veth interfaces:

    sudo -E ./tools/test_synapse_nfs.py                       # every tested NF, every workload
    sudo -E ./tools/test_synapse_nfs.py --nfs kvs smartcookie --churns 0 --zipf-params 0.0 1.2
    sudo -E ./tools/test_synapse_nfs.py --report results.csv --resume

The solutions are expected to be synthesized already (tools/synapse_batcher.py --synthesize); each
one is built (bf-p4c + controller) by the test harness unless --skip-build is given. The workload
sweep emits the same program many times over, so only one solution per distinct (P4, controller) is
tested unless --exhaustive is given. The tests run one after the other: there is one model. Logs go
under /tmp/synapse-tests/<solution>/.
"""

from __future__ import annotations  # keep 3.9+ annotation syntax valid on Python 3.8 (tofino2)

import csv
import os
import shutil
import subprocess
import sys
import time
from argparse import ArgumentParser
from dataclasses import dataclass
from hashlib import md5
from itertools import product
from pathlib import Path
from typing import Optional

TOOLS_DIR = Path(__file__).resolve().parent
PROJECT_DIR = TOOLS_DIR.parent
TESTS_DIR = PROJECT_DIR / "tests"
SYNTHESIZED_DIR = PROJECT_DIR / "synthesized"
TOFINO_TOOLS_DIR = PROJECT_DIR / "tofino" / "tools"
LOG_ROOT = Path("/tmp/synapse-tests")

sys.path.insert(0, str(TESTS_DIR))
import testbed  # noqa: E402  (tests/testbed.py: the harness the tests bring the model up with)

DEFAULT_TOTAL_FLOWS = [40_000]
DEFAULT_CHURN_FPM = [0, 1_000, 10_000, 100_000, 1_000_000]
DEFAULT_ZIPF_PARAMS = [0.0, 0.2, 0.4, 0.6, 0.8, 1.0, 1.2]
MAX_TPUT_HEURISTIC = "max-tput"
GALLIUM_HEURISTIC = "gallium"  # One solution per NF, gallium-<nf> (tools/synapse_batcher.py).
DEFAULT_HEURISTICS = [MAX_TPUT_HEURISTIC, GALLIUM_HEURISTIC]

# A bf-p4c build takes minutes (SmartCookie's about four), the tests themselves seconds to a minute.
DEFAULT_TIMEOUT_SEC = 40 * 60


@dataclass
class NF:
    name: str
    # The test's own conventions, where a solution's deployment differs from the test's default
    # (tests/<nf>.py reads them from the environment).
    env: dict[str, str]


# The NFs with a test. echo and fwd have none.
NFS = {
    "cl": NF("cl", {}),
    "fw": NF("fw", {}),
    "nat": NF("nat", {}),
    "kvs": NF("kvs", {}),
    "psd": NF("psd", {}),
    "pol": NF("pol", {}),
    "hyperloglog": NF("hyperloglog", {}),
    # The synthesized SmartCookie keeps the server on device 0, front panel port 1
    # (configs/tofino2-smartcookie.toml); the test's default is the hand-written program's layout.
    "smartcookie": NF("smartcookie", {"SC_SERVER_PORT": "1", "SC_SERVER_DEV": "0"}),
}


@dataclass
class Solution:
    nf: NF
    name: str

    @property
    def p4(self) -> Path:
        return SYNTHESIZED_DIR / f"{self.name}.p4"

    @property
    def cpp(self) -> Path:
        return SYNTHESIZED_DIR / f"{self.name}.cpp"

    @property
    def report(self) -> Path:
        return SYNTHESIZED_DIR / f"{self.name}.json"


@dataclass
class Result:
    solution: Solution
    status: str  # PASSED, FAILED, BUILD FAILED, TIMEOUT, ERROR
    scenarios: int
    seconds: float
    detail: str = ""


def build_synapse_nf_name(nf: str, total_flows: int, churn: int, zipf: float) -> str:
    """The batcher's naming (tools/synapse_batcher.py get_pcap_base_name)."""
    dist = "unif" if zipf == 0.0 else "zipf" + str(int(zipf) if int(zipf) == zipf else zipf).replace(".", "_")
    return f"{nf}-f{total_flows}-c{churn}-{dist}-h{MAX_TPUT_HEURISTIC}"


def panic(msg: str) -> None:
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(2)


def installed_p4_conf(name: str) -> Path:
    """The switch configuration `make install-tofino2` leaves in the SDE, which the model loads."""
    return Path(os.environ["SDE_INSTALL"]) / "share" / "p4" / "targets" / "tofino2" / f"{name}.conf"


def preflight(solutions: list[Solution], skip_build: bool) -> None:
    """Everything a run needs, checked before the first model comes up."""
    problems = []

    if os.geteuid() != 0:
        problems.append("must run as root (sudo -E): the tests bring the model up on veth interfaces")

    env = os.environ
    for var in ("SDE", "SDE_INSTALL"):
        if var not in env:
            problems.append(f"{var} is not set (run where the SDE is installed, with sudo -E)")
        elif not Path(env[var]).is_dir():
            problems.append(f"{var}={env[var]} is not a directory")
    if "SDE" in env and "SDE_INSTALL" in env:
        for tool in (Path(env["SDE"]) / "run_tofino_model.sh", Path(env["SDE"]) / "run_switchd.sh", Path(env["SDE_INSTALL"]) / "bin" / "bf-p4c"):
            if not tool.is_file():
                problems.append(f"missing {tool}")

    for script in (TOFINO_TOOLS_DIR / "veth_setup.sh", TOFINO_TOOLS_DIR / "Makefile", TOFINO_TOOLS_DIR / "ports_tof2.json"):
        if not script.is_file():
            problems.append(f"missing {script}")

    try:
        import scapy  # noqa: F401
    except ImportError:
        problems.append("python3 cannot import scapy (the tests build their packets with it)")

    for nf in {s.nf.name for s in solutions}:
        if not (TESTS_DIR / f"{nf}.py").is_file():
            problems.append(f"no test for {nf}: {TESTS_DIR / f'{nf}.py'}")
    for solution in solutions:
        for f in (solution.p4, solution.cpp):
            if not f.is_file():
                problems.append(f"missing {f} (synthesize it first: tools/synapse_batcher.py --synthesize)")
        if skip_build:
            if not testbed.controller_binary(solution.name).is_file():
                problems.append(f"--skip-build but {solution.name}'s controller was never built ({testbed.controller_binary(solution.name)})")
            if "SDE_INSTALL" in env and not installed_p4_conf(solution.name).is_file():
                problems.append(f"--skip-build but {solution.name}'s P4 is not installed in the SDE ({installed_p4_conf(solution.name)})")

    LOG_ROOT.mkdir(parents=True, exist_ok=True)
    free_gb = shutil.disk_usage(LOG_ROOT).free / 1e9
    if free_gb < 5:
        problems.append(f"only {free_gb:.1f} GB free on {LOG_ROOT}'s filesystem; a bf-p4c build needs a few")

    if problems:
        panic("cannot run the tests:\n  - " + "\n  - ".join(problems))

    model, controller = testbed.running_model(), testbed.running_controller()
    if model or controller:
        print(f"[*] a testbed is running (model: {model}, controller: {controller}); each test restarts it", flush=True)


def distinct_by_content(solutions: list[Solution]) -> list[Solution]:
    """Drop every solution whose emitted P4 and controller are byte-identical to one already kept:
    the same two files on the same model run the same test, so testing both only costs a build.
    The workload sweep produces many such duplicates (252 solutions, 67 distinct programs)."""
    seen: set[bytes] = set()
    kept = []
    for solution in solutions:
        p4 = SYNTHESIZED_DIR / f"{solution.name}.p4"
        controller = SYNTHESIZED_DIR / f"{solution.name}.cpp"
        if not (p4.is_file() and controller.is_file()):
            kept.append(solution)  # Let preflight report it missing.
            continue
        digest = md5(p4.read_bytes() + controller.read_bytes()).digest()
        if digest not in seen:
            seen.add(digest)
            kept.append(solution)
    return kept


def select_solutions(args) -> list[Solution]:
    solutions: list[Solution] = []
    for nf_name in args.nfs:
        nf = NFS[nf_name]
        if MAX_TPUT_HEURISTIC in args.heuristics:
            names = [build_synapse_nf_name(nf_name, flows, churn, zipf) for flows, churn, zipf in product(args.total_flows, args.churns, args.zipf_params)]
            solutions += [Solution(nf, name) for name in names]
        if GALLIUM_HEURISTIC in args.heuristics:
            solutions.append(Solution(nf, f"{GALLIUM_HEURISTIC}-{nf_name}"))
    if not args.exhaustive:
        solutions = distinct_by_content(solutions)
    return solutions


# bf_switchd's initialization can fail right after the previous one was stopped
# ("Resource temporarily not available, try again later"): its resources are still being released.
SWITCHD_RETRY_MSG = "Failed to initialize libbf_switchd"
SWITCHD_RETRY_DELAY_SEC = 10


def run_one(solution: Solution, skip_build: bool, timeout_sec: int) -> Result:
    result = run_once(solution, skip_build, timeout_sec)
    if result.status == "ERROR" and SWITCHD_RETRY_MSG in result.detail:
        time.sleep(SWITCHD_RETRY_DELAY_SEC)
        result = run_once(solution, True, timeout_sec)  # Built already by the first attempt.
    return result


def run_once(solution: Solution, skip_build: bool, timeout_sec: int) -> Result:
    cmd = [sys.executable, str(TESTS_DIR / f"{solution.nf.name}.py"), "--up", "--quiet", "--p4", str(solution.p4), "--controller", str(solution.cpp)]
    if skip_build:
        cmd.append("--no-build")
    env = dict(os.environ)
    env.update(solution.nf.env)

    logdir = testbed.nf_log_dir(solution.name)
    logdir.mkdir(parents=True, exist_ok=True)
    output = logdir / "test.log"
    start = time.monotonic()
    with open(output, "w") as out:
        try:
            proc = subprocess.run(cmd, cwd=PROJECT_DIR, env=env, stdout=out, stderr=subprocess.STDOUT, timeout=timeout_sec)
            returncode: Optional[int] = proc.returncode
        except subprocess.TimeoutExpired:
            returncode = None
    seconds = time.monotonic() - start
    testbed.down()  # Whatever happened, the next test starts from nothing.

    text = output.read_text(errors="replace")
    scenarios = sum(1 for line in text.splitlines() if line.startswith("[*] "))
    if returncode is None:
        return Result(solution, "TIMEOUT", scenarios, seconds, f"after {timeout_sec}s; logs in {logdir}")
    if returncode == 0 and f"{solution.name}: PASSED" in text:
        return Result(solution, "PASSED", scenarios, seconds)
    if "make install-tofino2 failed" in text or "make controller-debug failed" in text:
        return Result(solution, "BUILD FAILED", scenarios, seconds, f"see {testbed.build_log_file(solution.name)}")
    if "*** TEST FAILED ***" in text:
        # The scenario being run when it failed is the last one announced.
        last = [line[4:] for line in text.splitlines() if line.startswith("[*] ")]
        where = last[-1] if last else "?"
        return Result(solution, "FAILED", scenarios, seconds, f'at "{where}"; logs in {logdir}')
    lines = [line for line in text.splitlines() if line.startswith("ERROR")]
    detail = lines[-1] if lines else f"exit {returncode}"
    if "tofino-model start" in detail and not installed_p4_conf(solution.name).is_file():
        detail += f" (the P4 is not installed in the SDE: {installed_p4_conf(solution.name)})"
    return Result(solution, "ERROR", scenarios, seconds, f"{detail}; logs in {logdir}")


def load_passed(report: Path) -> set[str]:
    if not report.is_file():
        return set()
    with open(report) as f:
        return {row["name"] for row in csv.DictReader(f) if row["status"] == "PASSED"}


def main() -> int:
    parser = ArgumentParser(description=__doc__.strip().splitlines()[0])
    parser.add_argument("--nfs", choices=NFS.keys(), nargs="+", default=list(NFS.keys()), help="NFs to test (default: every NF with a test)")
    parser.add_argument("--total-flows", type=int, nargs="+", default=DEFAULT_TOTAL_FLOWS)
    parser.add_argument("--churns", type=int, nargs="+", default=DEFAULT_CHURN_FPM, help="churn rates (fpm) of the workloads")
    parser.add_argument("--zipf-params", type=float, nargs="+", default=DEFAULT_ZIPF_PARAMS, help="zipf parameters of the workloads (0.0: uniform)")
    parser.add_argument("--heuristics", choices=DEFAULT_HEURISTICS, nargs="+", default=DEFAULT_HEURISTICS)
    parser.add_argument("--exhaustive", action="store_true", help="also test the solutions whose P4 and controller are byte-identical to another one")
    parser.add_argument("--skip-build", action="store_true", help="the solutions are built already (tools/synapse_nfs_builder.py)")
    parser.add_argument("--timeout", type=int, default=DEFAULT_TIMEOUT_SEC, help="seconds a build plus test may take")
    parser.add_argument("--report", type=Path, help="write a CSV with one row per solution")
    parser.add_argument("--resume", action="store_true", help="skip the solutions --report already records as PASSED")
    parser.add_argument("--dry-run", action="store_true", help="list the solutions that would be tested")
    args = parser.parse_args()

    solutions = select_solutions(args)
    if args.resume:
        if not args.report:
            panic("--resume needs --report")
        passed = load_passed(args.report)
        solutions = [s for s in solutions if s.name not in passed]

    print(f"[*] {len(solutions)} solutions to test", flush=True)
    if args.dry_run:
        for s in solutions:
            print(f"    {s.name}")
        return 0

    preflight(solutions, args.skip_build)

    results: list[Result] = []
    rows_written = args.resume and args.report and args.report.is_file()
    report_file = open(args.report, "a" if rows_written else "w", newline="") if args.report else None
    writer = csv.writer(report_file) if report_file else None
    if writer and not rows_written:
        writer.writerow(["nf", "name", "status", "scenarios", "seconds", "detail"])

    try:
        for i, solution in enumerate(solutions, 1):
            print(f"[{i}/{len(solutions)}] {solution.name} ...", end=" ", flush=True)
            result = run_one(solution, args.skip_build, args.timeout)
            results.append(result)
            summary = f"{result.status} {result.scenarios} scenarios in {result.seconds / 60:.1f} min"
            print(summary + (f" ({result.detail})" if result.detail else ""), flush=True)
            if writer:
                writer.writerow([solution.nf.name, solution.name, result.status, result.scenarios, f"{result.seconds:.0f}", result.detail])
                report_file.flush()
    except KeyboardInterrupt:
        print("\n[*] interrupted; tearing the testbed down", flush=True)
        testbed.down()
    finally:
        if report_file:
            report_file.close()

    print("\n==================== summary ====================")
    failed = [r for r in results if r.status != "PASSED"]
    for nf_name in args.nfs:
        mine = [r for r in results if r.solution.nf.name == nf_name]
        if mine:
            ok = sum(1 for r in mine if r.status == "PASSED")
            print(f"{nf_name:12s} {ok}/{len(mine)} passed")
    for r in failed:
        print(f"  {r.status:12s} {r.solution.name}: {r.detail}")
    print(f"{len(results) - len(failed)}/{len(results)} passed")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
