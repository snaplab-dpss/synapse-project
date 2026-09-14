#!/usr/bin/env python3
"""Drive synapse's interactive `--walk` with a decision file and a few rules.

The walk prompts at every search step with more than one child. Everything the decision file
already holds for a visit replays without a prompt; at every other prompt this driver answers:

- the single child that is not a controller hand-off, a recirculation, a crossing or a reordered
  plan (`[R: ...]`): with a concat rotate and its shift form both offered, the concat rotate;
- when no compute child is offered, the gress is full in the model: `SendToEgress` if offered,
  else `Recirculate` (an "auto-cut");
- otherwise, or at a breakpoint node, it prints the step and quits, so the decision can be added
  to the file by hand and the run repeated.

The decisions synapse consumes are appended to the decision file, so a completed run leaves a
file that `--walk-replay` replays without prompting. The SmartCookie fixture
(`tofino/exp-compute/smartcookie-walk.txt`) was produced this way; its hand decisions are the
ground truth's cuts (`GT-DECISIONS.md`). A change to the placement model moves the auto-cuts and
makes the old file stop replaying: re-drive it from the hand decisions rather than edit it.

usage: walk.py --dir DIR [--decisions FILE] [--name NAME] [--bdd BDD] [--config CFG]
               [--profile PROF] [--synapse BIN] [--break NODE ...]
DIR receives the run's outputs (NAME.p4/.cpp/.json, the dots, `walk.out`, `walk.err`); the
decision file defaults to DIR/decisions.txt. The other defaults are SmartCookie's.
"""
import argparse
import re
import subprocess
from pathlib import Path

SKIP = {"Tofino_SendToController", "Tofino_Recirculate", "Tofino_SendToEgress"}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dir", required=True)
    parser.add_argument("--decisions")
    parser.add_argument("--name", default="sc-walk")
    parser.add_argument("--bdd", default="bdds/smartcookie.bdd")
    parser.add_argument("--config", default="configs/tofino2-smartcookie.toml")
    parser.add_argument("--profile", default="profiles/smartcookie-f40000-c0-unif.json")
    parser.add_argument("--synapse", default="synapse/build/bin/synapse")
    parser.add_argument("--break", dest="breakpoints", type=int, nargs="*", default=[])
    args = parser.parse_args()

    out = Path(args.dir)
    out.mkdir(parents=True, exist_ok=True)
    decisions = Path(args.decisions) if args.decisions else out / "decisions.txt"
    decisions.touch()
    breakpoints = set(args.breakpoints)

    cmd = [args.synapse, "--in", args.bdd, "--config", args.config, "--heuristic", "max-tput", "--profile", args.profile, "--seed", "0",
           "--out", str(out), "--name", args.name, "--walk", str(decisions)]
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=open(out / "walk.out", "w"), stderr=subprocess.PIPE, text=True, bufsize=1)

    block = []
    auto = 0
    picks = {}  # step -> (node, module, next, why) for the picks this driver answered
    with open(out / "walk.err", "w") as errlog:
        while True:
            line = p.stderr.readline()
            if not line:
                break
            errlog.write(line)
            errlog.flush()
            if line.startswith("[walk] step"):
                block = [line]
                continue
            if not line.startswith("[walk] pick ["):
                block.append(line)
                continue

            text = "".join(block)
            m = re.search(r"\[walk\] step (\d+): plan (\d+) at BDD node (\d+):", text)
            step, node = int(m.group(1)), int(m.group(3))
            offers = re.findall(r"\[(\d+)\] (\S+) \((\S+)\)\s+node=(\d+)\s+next=(\S+)(.*?)score=(<[^>]*>)", text)
            cands = [o for o in offers if o[1] not in SKIP and "[R:" not in o[5]]
            if len(cands) == 2 and {c[1] for c in cands} == {"Tofino_RotateLeft", "Tofino_RotateLeftShifts"}:
                cands = [c for c in cands if c[1] == "Tofino_RotateLeft"]
            if node not in breakpoints and len(cands) == 0:
                for kind in ("Tofino_SendToEgress", "Tofino_Recirculate"):
                    cut = [o for o in offers if o[1] == kind]
                    if cut:
                        cands = cut[:1]
                        print(f"auto-cut step {step:3d} node {node:4d}: {kind.replace('Tofino_', '')}")
                        picks[step] = (node, kind, cut[0][4], "driver: no compute child fit this gress")
                        break
            if node in breakpoints or len(cands) != 1:
                print(text.rstrip())
                print(f"== decision needed at step {step}, node {node}; {auto} auto picks this run ==")
                p.stdin.write("q\n")
                p.stdin.flush()
                break
            o = cands[0]
            print(f"auto step {step:3d} node {node:4d}: {o[1].replace('Tofino_', ''):24s} next={o[4]}")
            picks.setdefault(step, (node, o[1], o[4], "driver: the single child"))
            p.stdin.write(o[0] + "\n")
            p.stdin.flush()
            auto += 1

        rest = p.stderr.read()
        errlog.write(rest)
    p.wait()
    tail = [l for l in rest.splitlines() if l.startswith("[walk]") or "Tput:" in l or "Recirculations" in l]
    print("\n".join(tail[-6:]))
    write_replay(out / "walk.err", picks, out / "replay.txt")
    print(f"exit={p.returncode}")


def write_replay(err_log: Path, picks: dict, replay: Path) -> None:
    """The decisions in visit order, one line per step, as `--walk-replay` consumes them.

    The decision file synapse appends to keeps the replayed lines where they were and adds the
    prompted ones at the end, which does not replay (a stale line is met before its visit). The
    log has every step: a replayed one names its pick, a prompted one is answered by this
    driver, which remembers what it sent for that step.
    """
    lines = []
    step = None
    for line in open(err_log):
        m = re.match(r"\[walk\] step (\d+): plan \d+ at BDD node (\d+):", line)
        if m:
            step = (int(m.group(1)), int(m.group(2)))
            if step[0] in picks:
                node, module, nxt, why = picks[step[0]]
                lines.append(f"node={node} module={module} next={nxt}  # {why}, step {step[0]}")
            continue
        m = re.match(r"\[walk\] replayed: \[\d+\] (\S+) next=(\S+)", line)
        if m and step and step[0] not in picks:
            lines.append(f"node={step[1]} module={m.group(1)} next={m.group(2)}  # replayed, step {step[0]}")
    with open(replay, "w") as f:
        f.write("# Decisions in visit order, for --walk-replay.\n")
        f.write("\n".join(lines) + "\n")
    print(f"replay file: {replay} ({len(lines)} decisions)")


if __name__ == "__main__":
    main()
