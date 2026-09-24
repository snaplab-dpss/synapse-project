# Meta4 (expert baseline)

Tofino implementation of **Meta4**: per-domain-name traffic accounting in the
switch data plane, from Princeton's
[p4-projects](https://github.com/Princeton-Cabernet/p4-projects/tree/master/Meta4).
Meta4 watches DNS responses to learn which client-server flows belong to which
domain name, then attributes every subsequent data packet to that domain —
all without sending traffic to a controller.

Upstream docs are preserved in [`README.upstream.md`](README.upstream.md).

## Credit

**All credit for the Meta4 design and implementation belongs to its original
authors.** This directory is an adaptation of existing work, not our own; our
contribution is limited to the portability fixes and the one parameter
correction described below.

- **Original authors:** Jason Kim, Hyojoon Kim and Jennifer Rexford, Princeton
  University.
- **Paper:** *Analyzing Traffic by Domain Name in the Data Plane*, ACM SOSR 2021
  ([dl.acm.org](https://dl.acm.org/doi/10.1145/3482898.3483357)).
- **Source:** [Princeton-Cabernet/p4-projects](https://github.com/Princeton-Cabernet/p4-projects),
  directory `Meta4/`, file `P4/netassay_v4_j6.p4` — the traffic-volume
  measurement version, which is the one the paper evaluates. (Upstream also
  ships `netassay_iot_j6.p4` for IoT fingerprinting and
  `netassay_tunnel_j{7,8}.p4` for DNS-tunnel detection; those are different
  applications built on the same machinery.) If you use this code, please cite:

  ```bibtex
  @inproceedings{kim2021meta4,
      title={Analyzing Traffic by Domain Name in the Data Plane},
      author={Kim, Jason and Kim, Hyojoon and Rexford, Jennifer},
      booktitle={Proceedings of the ACM SIGCOMM Symposium on SDN Research (SOSR)},
      pages={1--12},
      year={2021}
  }
  ```

- **License:** MIT, Copyright (c) 2021 Jason Kim ([`LICENSE`](LICENSE)),
  preserved unchanged.

## What it is

Meta4 answers "how much traffic did each domain name account for?" entirely in
the data plane. A controller would have to see every DNS response, so instead
the switch does the association itself:

| Stage | Structure | What it does |
|-------|-----------|--------------|
| A | `known_domain_list` (match-action) | Matches the parsed domain name against the watch list, yielding a domain ID (`dID`) |
| B | DNS Response Table (registers) | Maps `<client IP, server IP>` → `<dID, timestamp>`, so later data packets can be attributed |
| C | `dns_total_queried` / `dns_total_missed` | Per-domain DNS response counters, including responses that could not be stored |
| D | `packet_counts_table` / `byte_counts_table` (egress) | Per-domain packet and byte counters |

**DNS responses** are parsed in the ingress parser (up to four labels of 15
bytes each, per Table 1 of the paper), matched against the watch list, and if
known, the `<cIP, sIP>` pair is inserted into the DNS Response Table. **Data
packets** hash their `<srcIP, dstIP>` pair into the same table; on a hit, the
domain ID travels to egress in a custom `netassay_hdr` header and increments
that domain's counters.

The DNS Response Table is the interesting part. It is a **two-stage hash table
with lazy timeout eviction**: each stage is an independent hash of the address
pair, and an entry may be overwritten once it has gone untouched for the
timeout (100 s). Insertion tries stage 1, and if that is occupied by a live
entry, the packet is **resubmitted** to try stage 2 — which is why the program
carries a `resubmit_data_t` header and a `stage_indicator` bit. Data packets
that match refresh the timestamp, so active sessions are not evicted.

## Layout

```
p4/meta4.p4               data plane (upstream netassay_v4_j6.p4 + the edits below)
p4/meta4-resources.txt    bf-p4c resource usage on Tofino 2
p4/Makefile               APP := meta4; includes tofino/tools/Makefile
known_domains_v1.txt      watch list of domain names -- OUR eval configuration, see change 6
allowed_dns_dst.txt       client prefixes whose DNS responses are tracked -- empty, see change 6
banned_dns_dst.txt        client prefixes whose DNS responses are ignored -- ours, see change 6
```

## Changes vs. upstream

Upstream states that `netassay_v4_j6.p4` "compiles with SDE v9.2.0". We build
with **bf-p4c 9.13.4**, which rejects four things 9.2.0 accepted. The DNS
parsing, the domain matching, the DNS Response Table algorithm, the resubmit
logic and the counters are **all unchanged**; items 1-4 are portability fixes
and item 5 is a parameter correction back to the paper's stated value.

Items 2-4 were each checked against both targets: **Tofino 1 rejects them under
9.13.4 exactly as Tofino 2 does**, so they are SDE-version issues, not
Tofino 1 → Tofino 2 porting issues.

1. **Tofino 2 target.** `#include <tna.p4>` →
   `#if __TARGET_TOFINO__ == 2 #include <t2na.p4> #else #include <tna.p4> #endif`.

2. **Explicit `Register` / `RegisterAction` index types.** bf-p4c 9.13.4 no
   longer infers the `_` index placeholder: 12 `Register<T,_>` and 21
   `RegisterAction<T,_,U>` declarations got their index type spelled out
   (`bit<32>`, or `bit<16>` for the three egress counter registers, matching
   the width each is actually indexed with). Pure type annotations — no
   behaviour change.

3. **Byte-aligned resubmit header.** `resubmit_data_skimmed_t` was a bare
   `bit<1> stage_indicator`. The deparser emits this header on a resubmit, and
   9.13.4 rejects a header that is not byte-aligned
   (`error: Tofino requires byte-aligned headers`), so it gained
   `bit<7> _padding0`. The 7 pad bits line up with the first byte of
   `resubmit_data_t`, which is what the parser reads back, so the resubmit path
   is unchanged.

4. **Hash inputs precomputed into metadata.** The four hash calls were written

   ```p4
   hash_1.get(headers.ipv4.src + headers.ipv4.dst + 32w134140211)
   ```

   A Tofino action executes all of its instructions **in parallel**, so an
   operand written by another instruction of the same action has no defined
   value; `a + b + salt` is two dependent `add`s and 9.13.4 rejects it with
   *"action spanning multiple stages"*. Three points make this unavoidable
   rather than a matter of rewriting the expression:

   - it is not about the hash — the same two adds fail with no hash present at
     all;
   - the sum cannot move into the hash unit (`@in_hash`), because that unit is
     a GF(2) matrix: bf-p4c's `CanBeIXBarExpr` admits slices, concatenations,
     casts, XOR and masking, and rejects everything else, `Add` included. XOR
     is free there, a carrying `+` never is;
   - splitting the statement is not enough either, because copy propagation
     folds the two adds back into one action, even across an intervening table.

   So the sum is built by two actions, `set_hash_base_{dns,ip}` and
   `salt_hash_inputs`, called from the apply block, leaving
   `ig_md.hash_in_{1,2}` holding **exactly the sums that used to be written
   inline** — the hashes see bit-for-bit the same input as upstream. The
   precompute sits at the top of each branch so the two extra ALU operations
   overlap the domain tables instead of deepening the pipeline. This costs no
   resources: with the adds removed entirely the program places identically.

   The candidate shapes and the p4c passes behind each rejection are worked
   out in [`tofino/experiments/parallel-action-add`](../experiments/parallel-action-add).

   The salts stay as they are. They are load-bearing: an additive salt
   introduces carries, which genuinely decorrelates the two hashes. Measured
   over the address pairs, 334,586 collisions in stage 1 left only **12** that
   also collide in stage 2 (chance ≈ 5). Replacing them with two different CRC
   polynomials would be a redesign, not a port.

5. **Paper-sized DNS Response Table: 32768 entries per stage (was 65536).**
   This is the one change that is not a port. Upstream applies
   `#define TABLE_SIZE 65536` to *each* of the two stages, i.e. 2^17 entries in
   total. The paper's Table 1 gives the hardware implementation as **"DRT
   Length 2^16 entries, DRT Stages 2"**, and §5.3 is explicit that the stage
   count "does not vary the total amount of memory; it just splits the total
   memory across stages" — so 2^16 total over two stages is 2^15 each. At
   upstream's doubled size the program does not fit: `sip_cip_reg_1` fills its
   stage and the third table that must attach to it has nowhere to go. At the
   paper's size it compiles with room to spare (11 of 20 stages). We use the
   paper's value, which is both the documented configuration and the one that
   builds.

6. **The three list files hold our evaluation configuration, not upstream's.**
   Upstream's `known_domains_v1.txt` (313 names from the Princeton campus
   trace), `banned_dns_dst.txt` (15 campus hosts) and `allowed_dns_dst.txt`
   (8 campus prefixes) were chosen for their deployment; their controller uses
   the last two as an allow-list (everything banned, the campus prefixes
   allowed), which would refuse every DNS response in synthetic traffic. Ours
   are the watch list our C NF is built with (32 names, identical to
   `dpdk-nfs/meta4/domains.txt`), a single ignored prefix (`10.9.0.0/24`), and
   an empty allow-list, so that both implementations are configured
   identically for the comparison. The originals are in
   `tofino/princeton-p4-projects/Meta4-tofino/`. Configuration only; no
   effect on what the data plane does per packet.

### Status on Tofino 1

With the same edits the program still fails to place on Tofino 1 (12 stages):
`domain_reg_2` is read from the data-packet path and written from the DNS
resubmit path, and those two sites sit at depths the compiler cannot bring into
the register's single stage. This is not caused by any of the changes above —
it reproduces with the hash arithmetic removed entirely. Our DUT is Tofino 2,
so we have not pursued it.

## Resources (Tofino 2, bf-p4c 9.13.4)

11 of 20 stages, 0 errors. Full report in
[`p4/meta4-resources.txt`](p4/meta4-resources.txt).

| Resource | Used |
|----------|------|
| Stages | 11 / 20 |
| SRAM | 85 |
| Map RAM | 80 |
| TCAM | 49 |
| Exact match input xbar | 115 |
| Ternary match input xbar | 132 |
| Hash bits | 349 |
| Hash dist units | 18 |
| Gateways | 33 |
| VLIW instructions | 26 |
| Meter ALUs | 11 |
| Logical table IDs | 45 |

The two 2^15-entry DNS Response Table stages dominate: stages 5/6 and 8/9 each
carry ~18 SRAM and ~18 map RAM blocks for `sip_cip`, `domain` and `tstamp`.
Of the 49 TCAM blocks, 48 are the domain-name matching (`known_domain_list`,
ternary over the four parsed label fields, split 24/24 across stages 0 and 1)
and 1 is the `lpm` client-IP list (`banned_dns_dst`).

## Running

Compile and install (Tofino 2):

```
cd tofino/meta4/p4 && make install-tofino2
```

Resource report:

```
cd tofino/meta4/p4 && ../../tools/get_resources_tofino2.sh meta4.p4
```

## Not done yet

- **Controller.** Nothing populates `known_domain_list`, `allowed_dns_dst` or
  `banned_dns_dst` at runtime. Upstream's `knownlist_v4_j6.py` generates the
  match-action rules from `known_domains_v1.txt`; it has not been ported to our
  `libsycon`-based harness, so there is no `meta4.py` here yet.
- **Evaluation.** Not wired into `eval/` — the traffic generator emits UDP, and
  a Meta4 benchmark needs DNS responses followed by data packets on the learned
  flows.
