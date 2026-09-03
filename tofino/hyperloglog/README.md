# HyperLogLog (expert baseline)

Tofino implementation of the [HyperLogLog](https://en.wikipedia.org/wiki/HyperLogLog)
approximate distinct-counting data structure, from Princeton's
[p4-projects](https://github.com/Princeton-Cabernet/p4-projects) (a byproduct of
the BeauCoup project). We use it as an **expert baseline** to compare against
synapse-synthesized solutions, alongside `tofino/netcache` and
`tofino/switcharoo`.

Upstream docs are preserved in [`README.upstream.md`](README.upstream.md).

## Credit

**All credit for the HyperLogLog data-plane design and implementation belongs to
its original author.** This directory is an adaptation of existing work, not our
own; our contribution is limited to the small portability edits and evaluation
harness described below.

- **Original author:** Xiaoqi Chen, Princeton University
  (`xiaoqic [at] cs.princeton.edu`).
- **Source:** [Princeton-Cabernet/p4-projects](https://github.com/Princeton-Cabernet/p4-projects),
  directory `HyperLogLog-tofino/`.
- **Origin:** a byproduct of the **BeauCoup** project. If you use this code,
  please cite it:

  ```bibtex
  @article{chen2020beaucoup,
      title={BeauCoup: Answering Many Network Traffic Queries, One Memory Update at a Time},
      author={Chen, Xiaoqi and Feibish, Shir Landau and Braverman, Mark and Rexford, Jennifer},
      journal={ACM SIGCOMM 2020},
      year={2020},
      publisher={ACM}
  }
  ```

- **License:** Copyright 2020 Xiaoqi Chen, Princeton University. Released under
  the **GNU Affero General Public License v3** ([`LICENSE`](LICENSE)). The AGPLv3
  is preserved unchanged. Per its terms, our modifications to the P4 data plane
  (see [Changes vs. upstream](#changes-vs-upstream)) are made available here
  under the same license.

## Layout

```
hyperloglog.py            controller (brings up the front-panel ports)
p4/hyperloglog.p4         data plane, GENERATED from the template (committed)
p4/hyperloglog.p4template Jinja template (source of truth for the .p4)
p4/p4gen.py               upstream code generator
p4/Makefile               APP := hyperloglog; includes tofino/tools/Makefile
```

## Generating the data plane

HyperLogLog is shipped as a Jinja template that is rendered into a concrete P4
program for a chosen number of estimators `M` and inverse-probability `scaling`:

```
python3 p4/p4gen.py p4/hyperloglog.p4template p4/hyperloglog.p4 -M 64 --scaling 20
```

(needs `jinja2`; `pip install -r p4/requirements.txt`. Note: the flag is `-M`,
not `--M` as the upstream README says.)

**We commit the generated `hyperloglog.p4`** rather than regenerating at
experiment time, because:

- `p4gen.py`'s `get_seed()` picks **random** hash seeds on every run, so
  regenerating would silently change the program (and its results) run to run.
  Committing one rendering makes experiments reproducible.
- Our build system (`tofino/tools/Makefile`, `eval/hosts/switch.py`) compiles a
  single `.p4`; nothing else in the repo depends on jinja2.

We use the upstream defaults **M = 64, scaling = 20** (α = 0.709…,
MAGNIFY_FACTOR = 3046596202). To change them, re-render with `p4gen.py` and
re-commit the `.p4`.

## Changes vs. upstream

The **core counting / estimation logic is unchanged.** The only edits (made in
the template, so the committed `.p4` regenerates cleanly) are portability fixes
needed to run on our Tofino2 DUT under our evaluation harness:

1. **Tofino2 target.** `#include <tna.p4>` →
   `#if __TARGET_TOFINO__ == 2 #include <t2na.p4> #else #include <tna.p4> #endif`.
2. **Portable port metadata.** `pkt.advance(64) // tofino 1` →
   `pkt.advance(PORT_METADATA_SIZE)` (arch-provided: 64 on Tofino1, 192 on
   Tofino2), matching our other apps.
3. **Explicit RegisterAction index types.** The newer compiler (bf-p4c 9.13.4)
   cannot infer the `_` index placeholder that the older Tofino1 compiler could,
   so four `RegisterAction<.., _, ..>` gained explicit index widths
   (`bit<6>` = `bit<LOG_M>` for the estimator register indexed by `bin_id`;
   `bit<32>` for the three single-cell registers indexed by a constant `0`).
   These match the annotations in upstream's own shipped generated file — pure
   type annotations, no behavioral change.

**Forwarding is upstream's own behavior:** the program already `reflect()`s each
packet back to its ingress port and writes the estimate into
`hdr.ethernet.src_addr`. That is exactly the "echo" methodology our TG uses
(`broadcast = dut_ports`, `symmetric = []`, `route = []`), so no forwarding
changes were needed.

## Running

Compile on the DUT (Tofino2):

```
APP=hyperloglog make -f ../../tools/Makefile install-tofino2
```

Throughput test (from `eval/`):

```
python3 test_hyperloglog.py
```

HyperLogLog is a **monitoring** primitive (it counts distinct IPv4 src/dst
pairs and reflects packets), not a key-value store, so — unlike NetCache and
Switcharoo — the test uses **no KVS server**.
