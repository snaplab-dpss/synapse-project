# The first synthesized SmartCookie of the ground truth's shape

`sc-walk.p4` is what synapse emitted for the plan `../smartcookie-walk.txt` replays to (see
`../PLAN.md` and `../GT-DECISIONS.md`, "Reached (2026-09-13)"): two crossings and one
recirculation on the SYN path, one more lap on the cookie-check path, the SipHash rounds in a
state header of 11 words per gress. bf-p4c's front end, PHV allocation and action constraints
pass (README, "One rotation per pair of words"); what stops it is the assembler: bf-p4c's own
table placement takes 32 stages against Tofino 2's 20, spreading the hash-unit tables that the
model packs six units to a stage. Phase 4 starts from this file.

Regenerate with the walk tool from the repository root:

```
synapse/build/bin/synapse --in bdds/smartcookie.bdd --config configs/tofino2-smartcookie.toml \
  --heuristic max-tput --profile profiles/smartcookie-f40000-c0-unif.json --seed 0 \
  --walk-replay tofino/exp-compute/smartcookie-walk.txt --out <dir> --name sc-walk
```
