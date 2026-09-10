#!/usr/bin/env bash
# run.sh <tag> [fix ...]   apply fixes to base.p4, compile, and report the outcome.
# Success is decided by the artifact, not by grepping the log: bf-p4c can print "0 errors" on a
# line of its own and still refuse to emit a binary ("Due to errors, no binary will be generated").
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
OUT=/home/fcp/synapse-project/synthesized
TAG="$1"; shift
python3 "$HERE/fixes.py" "$HERE/base.p4" "$OUT/$TAG.p4" "$@" || exit 1
cp "$HERE/base.cpp" "$OUT/$TAG.cpp"
docker exec nostalgic_chaum bash -lc \
  "cd /home/user/workspace/synthesized && APP=$TAG SDE_INSTALL=/home/user/bf-sde-9.13.4/install timeout 1800 make -f ../tofino/tools/Makefile install-tofino2" \
  > "/tmp/$TAG-make.log" 2>&1
docker exec nostalgic_chaum bash -lc "
  B=/home/user/bf-sde-9.13.4/build/p4-build/tofino2/$TAG/$TAG/tofino2
  L=/home/user/bf-sde-9.13.4/logs/p4-build/tofino2/$TAG/make.log
  # bf-rt.json is not proof: it is written while the compiler is still running, and a failed run
  # leaves sctest.bfa behind at zero bytes. The assembly being non-empty is the real signal.
  if [ -s \$B/pipe/$TAG.bfa ] && [ -s \$B/pipe/context.json ]; then V=COMPILED; else V=FAILED; fi
  printf '%-14s %-9s' '$TAG' \"\$V\"
  printf ' | stages i/e: '
  grep -m1 -A2 'Number of stages in table allocation' \$B/pipe/logs/table_summary.log 2>/dev/null \
    | grep -oE '[0-9]+\$' | paste -sd/ - | tr -d '\n'
  printf ' | '
  grep -oE '[0-9]+ field slices remain unallocated' \$L 2>/dev/null | tail -1 | tr -d '\n'
  printf ' | '
  grep -oE 'no binary will be generated' \$L 2>/dev/null | tail -1 | tr -d '\n'
  echo"
