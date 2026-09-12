# The first synthesized SmartCookie of the ground truth's shape

`sc-walk.p4` is what synapse emitted for the plan `../smartcookie-walk.txt` replays to (see
`../PLAN.md`, Phase 2): two crossings and one recirculation per client path, the SipHash chain
computed once per pass and called from both TCP paths, the `seq - 1` selected into the shared
field ahead of lap 2. It does not yet build: bf-p4c's front end and instruction selection pass,
and after fifteen minutes the back end stops on action constraints -- a rotate output the hash
unit cut six ways copied into `hdr.egress_state` at the crossing, and `build_recirc_hdr` packing
three PHV sources. Phase 3 starts from this file.

Regenerate with the walk tool from the repository root:

```
synapse/build/bin/synapse --in bdds/smartcookie.bdd --config configs/tofino2-smartcookie.toml \
  --heuristic max-tput --profile profiles/smartcookie-f40000-c0-unif.json --seed 0 \
  --walk-replay tofino/exp-compute/smartcookie-walk.txt --out <dir> --name sc-walk
```
