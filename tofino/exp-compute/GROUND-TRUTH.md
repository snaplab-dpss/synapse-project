# Ground-truth SmartCookie (`synthesized/smartcookie-manual.p4`)

A complete SmartCookie for Tofino 2, written by hand, that compiles **and passes the model test**
(`tests/smartcookie.py`). It is the reference the synthesized solution should be judged against:
what synapse ought to produce, and the yardstick for how far its output is from something the
target accepts.

It lives in `synthesized/` rather than here so it sits next to the machine-generated solutions it
is meant to be diffed against, and so the testbed can run it like any other NF:

```
bf-p4c --target tofino2 --arch t2na -o out synthesized/smartcookie-manual.p4   # 6 s
sudo -E tests/testbed.py up smartcookie-manual && sudo -E python3 tests/smartcookie.py
```

It follows synapse's own P4 template: the same `synapse_ingress_headers_t` and
`synapse_ingress_metadata_t` structs, the `cpu_h` / `recirc_h` / `recirc_state_h` headers, the
`hdr0..hdr4` layout that synapse's header guessing produces for ethernet, IPv4, TCP and UDP, the
`TofinoIngressParser` and `IngressParser` shape, the `fwd_op_t` enum, `ingress_port_to_nf_dev` and
the usual forwarding actions. A reader should be able to diff it against a synthesized file.

| | |
|---|---|
| stages | 17 ingress, 17 egress (of 20 each) |
| match tables | 47 |
| chain actions | 28 declarations (17 names, most written once per pipeline), executing 12 SipRounds |
| `@in_hash` sites | 8 |

## Semantics

Taken from `dpdk-nfs/smartcookie`, the NF synapse compiles, not from the paper:

- `cookie_hash` = HalfSipHash-2-4 over four words: source address, destination address,
  source port concatenated with destination port, and the sequence number.
- `cookie` = `cookie_time ^ cookie_hash`, with `cookie_time = (ticks(now) - stored_delta) >> 12`.
- A client SYN is answered with a SYN-ACK whose sequence number is the cookie and whose
  acknowledgement is the original sequence plus one, with addresses and ports swapped.
- A client non-SYN is passed to the server if the bloom filter already holds the flow, otherwise
  its cookie is checked: `cookie_val = (ack - 1) ^ hash(..., seq - 1)` and the packet is accepted
  only if `cookie_time - cookie_val` is 0, 1 or 2.
- An ECE-tagged packet from the server records its flow in the bloom filter and is dropped.
- Any other IPv4 packet is routed by the first octet of the destination address.
- A timesync packet from the server updates the stored delta.

## Structure

The twelve SipRounds are one body, written once per pipeline and executed twice per pipeline per
lap, two in ingress and two in egress. Twelve rounds therefore take three laps. The four state
words, the round counter, the callback type, the egress port and the cookie time travel in a
recirculated header; the message word for each lap is chosen by a table keyed on the counter.

This is the shape the whole investigation converged on, and each element of it was forced by a
measured failure. See `README.md` for the experiments behind them.

## What synapse cannot express today

Kept here so the gap is explicit. Ordered by how hard each looks.

1. **One body executed many times.** The chain is written once and re-executed on each lap, so
   three laps cost one lap of hardware. Synapse emits separate code per lap, because its BDD
   arrived from symbolic execution with the loop already unrolled and nothing marking the twelve
   rounds as iterations of one thing. This is the large gap: it needs the repeated structure
   recognised, a body emitted once, and the search and placer taught what that costs.
   *Consequence today: 92 distinct actions and about 160 live values, against 19 and 9 here.*

2. **Using the egress pipeline.** All synapse output lives in ingress. The ground truth needs
   egress for half the rounds, which is what brings the hash-unit demand inside budget. This is
   the smaller gap and it may matter more: the unrolled form has never been tried with the work
   split across both pipelines, and that experiment is still open.

3. **A loop counter and dispatch on it, needed only if the loop is rolled.** The rolled ground
   truth carries `round` in the recirculated header, does `round = round + 2` each half-lap, and
   keys tables on it to pick the message word and the finish action. That is a genuine runtime
   counter and synapse has no module for one.
   **The unrolled form does not need it.** Synapse already emits a pass identifier into the
   recirculation header (`build_recirc_hdr(N)` writing `hdr.recirc.code_path`), dispatches on it
   with an `if (hdr.recirc.code_path == N)` chain, and carries live variables across the boundary.
   `sc_unrolled.p4` was rewritten to use exactly that, with its bespoke counter field deleted, and
   still compiles in 7 s -- dispatching in *egress* as well as ingress. So for the direction we are
   taking, item 3 is not a new module at all. What is left of it belongs to item 2: **the egress
   parser has to extract the recirculation header and the egress control has to open with the same
   `code_path` chain the ingress has.**

4. **Emitting an operation into the hash unit deliberately.** Smaller than it looked, once tested.
   Synapse *already* has half of it: `RotateLeft::uses_hash_unit()` sends any rotate that is not a
   whole number of bytes through `@in_hash`, and `ComputeAction` budgets hash bits and hash
   distribution units. What is missing is the general case: **an expression tree that would span
   stages collapses into one hash-unit op**. `v0 ^ v1 ^ v2 ^ v3` costs three ALU instructions and
   an action cannot span stages ("xor: action spanning multiple stages"), but it is a single
   `@in_hash` op. That is the rule to add.
   Two other uses in an earlier version of this file turned out to be superstition: a slice-and-
   widen read of an intrinsic (`ctime = ingress_mac_tstamp[47:16]`) and a byte read feeding a table
   parameter (`nf_dev = dst[31:24]`) both compile as plain ALU ops and pass the model test. They
   have been removed, taking the program from ten `@in_hash` sites to eight.

5. **Respecting the immediate pathway, which is a per-table budget.** Not "one hash-producing
   action per table": the limit is **32 bits of hash-produced immediate data per table, summed over
   its actions**. Merging the two final-xor actions into one table gives
   "the number of bits required to go through the immediate pathway 64 ... is greater than the
   available bits 32", and bf-p4c then crashes with SIGSEGV rather than exiting cleanly.
   Synapse models this per *action* (`ComputeAction::MAX_HASH_BITS_PER_ACTION`), which happens to
   agree today only because it emits one action per keyless table. The accounting has to move to
   the table as soon as one table carries several actions, which is exactly what item 3's dispatch
   tables need.
   The claim that a hash operation cannot sit in a keyless table is **wrong**: every round action
   here holds an `@in_hash` and is called bare from the apply block, and bf-p4c compiles each into
   a keyless `hash_action` table. The bloom's `Hash.get()` queries are the same shape.

6. **Wide comparisons, where synapse is mostly right already.** `age > 2` on a 32-bit value does
   not fit a gateway ("condition too complex, limit of 4 bytes + 12 bits of PHV input exceeded"),
   so the three accepted epochs are constant table entries here. Measured, the gateway budget
   splits by comparison kind rather than by width alone:

   | condition | limit on PHV operands |
   |---|---|
   | `==`, `!=` | 4 bytes, so a 32-bit equality fits |
   | `<`, `>`, `<=`, `>=` against a power-of-two boundary | free at any width: it is a "high bits are zero" mask test |
   | `<`, `>`, `<=`, `>=` otherwise | **12 bits total**, and a constant operand costs nothing |

   Toys: `cmpk12.p4` (12-bit field vs a constant) compiles, `cmpk13.p4` (13 bits) does not, and
   `cmp12.p4` -- two 12-bit *fields*, 24 bits of operand -- does not either.

   Synapse already covers most of this. `If.cpp`'s `is_wide_const_inequality` diverts a relational
   against a constant wider than 8 bits away from the gateway, exempts the power-of-two case, and
   falls back to rewriting the comparison over narrow slices rather than punting to the controller.
   Two gaps remain, both latent (nothing shipped hits them):
   - the threshold is 8 bits where the hardware allows 12, so some conditions are split that need
     not be;
   - a relational between **two non-constant operands** is not checked at all: it bypasses
     `is_wide_const_inequality` (which requires a constant RHS) and passes
     `condition_meets_phv_limit`, which counts bytes against 4 without regard to comparison kind.
     Two 12-bit fields are 2 bytes and sail through, and bf-p4c then rejects the program. The check
     needs to sum the non-constant operand widths against 12 bits for relational comparisons.

7. **Deparser checksums. This is a live correctness bug, not a missing nicety.** Synapse emits no
   dataplane checksum at all: `Ignore.cpp` lists `nf_set_rte_ipv4_udptcp_checksum` among the calls
   it drops, and `ModifyHeader.cpp`'s `filter_out_checksum_mods` strips the checksum field write
   from the header modifications. So a packet the *dataplane* rewrites leaves with the checksum it
   arrived with; only packets that take the CPU path are corrected, by the controller's
   `update_ipv4_tcpudp_checksums`.
   Verified on a shipped solution: `nat-f40000-c0-unif-hmax-tput` with `NAT_INDEX_BYTE_ORDER=big`
   fails `tests/nat.py` on the fast path with both the IPv4 and the UDP checksum wrong, and passes
   as soon as `NAT_CHECK_CHECKSUMS=0` is set. Checksums are the only thing still failing there.

   The target's rule, measured both ways and asymmetric in a way that matters:
   - a `Checksum()` **input** that is a slice is a hard error, "unexpected type of parameter
     hdr.hdr1.data2[39:8] in Checksum";
   - a `Checksum()` **output** written to a slice compiles with 0 errors and is **silently
     ignored**, leaving the original checksum on the wire. This is how the first version of this
     program shipped a wrong checksum past the compiler.

   So every field a checksum touches must be a header field in its own right, which is a constraint
   on header guessing, not just on emission: the IPv4 protocol and header checksum are separate
   fields here, as are the TCP window, checksum and urgent pointer. The ground truth shows the
   whole shape of the fix, deparser `Checksum().update({...})` guarded by a flag the rewriting
   actions set.

Items 4 through 7 are ordinary emission rules synapse could adopt. Item 3 is a modest new module.
Items 1 and 2 are the real question, and 2 should be tested before 1 is attempted.

## What compiling cannot tell you

An earlier version of this program compiled in 4 s with 0 errors and was wrong in six ways. Five
are constraints the emitter has to respect; they are listed here because a synthesizer that only
checks "does bf-p4c accept it" will violate every one of them and never find out. The sixth was a
plain transcription slip on my part, hashing `ack` where the cookie check wants `ack - 1`, which
is the other reason `tests/smartcookie.py` exists.

1. **Statements inside an action run in order.** A two-field swap written as `a = b; b = a;`
   duplicates `b`. The old value has to be captured by an earlier action, because a temporary
   written and read inside one action makes it span stages, which bf-p4c does reject. A *single*
   whole-field write of a field in terms of itself is fine and is one operation:
   `ports = ports[15:0] ++ ports[31:16]` swaps correctly.
2. **The same ordering across tables.** `ack = seq + 1` has to read `seq` before the table that
   writes the cookie into it, so it is staged on the way into the pipeline.
3. **A deparser `Checksum()` can neither read nor write a slice.** The write is silently ignored
   and the packet keeps the checksum it arrived with, which is why item 7 above splits header
   fields instead of staging them into metadata: staging fixes the read side and does nothing for
   the write side.
4. **A recirculated packet still has to parse.** Marking a packet in flight by rewriting its
   ethertype made the ingress parser reject it on the way back round, so the second and third laps
   hashed nothing. The recirculation header's `code_path` is the marker; the ethertype is left
   alone.
5. **`f = C ++ (f[7:0] | K)` silently loses the OR.** This one is a bf-p4c bug, not a rule to
   follow: it allocates a temporary for the concat operand and never writes it
   (`set hdr.f.0-7, $concat_to_slice27`, with nothing anywhere assigning `$concat_to_slice27`), so
   the field comes out zero. Two slice assignments compile to `set hdr.f.8-15, C` plus
   `or B7, K, B7` and are correct. Reproducers: `concatE.p4` (broken), `concatF.p4` (correct).

None of these five appear in any solution synapse ships today: a scan of `synthesized/*.p4` finds
no in-action swap, no concat containing an operation, no deparser checksum and no ethertype
rewrite. So nothing is broken right now, but items 1 and 2 become live the moment the emitter is
taught to rewrite packets in place, and item 3 changes what header guessing has to produce before
a checksum can be emitted at all.

## Plan for changing synapse

Agreed order of attack (2026-09-08). It is not the order the list above is numbered in: that one
runs by difficulty, this one by dependency and by value.

| order | item | why here |
|---|---|---|
| 1 | 2, use the egress pipeline | gates item 1, and pays off beyond SmartCookie |
| 2 | 4 + 5, deliberate `@in_hash` and hash-action placement | mechanical, needed either way |
| 3 | 6, wide comparisons as table entries | mechanical, small |
| 4 | 7, checksums and splitting fields to suit them | self-contained, a correctness gap for any packet-crafting NF |
| 5 | 3, loop counter and dispatch | **dissolved**: synapse's `code_path` already does it; see below |
| 6 | 1, one body executed many times | the big one, and **no longer on the critical path**: see below |

The five rules under "What compiling cannot tell you" are not separate work; each attaches to
whichever item touches it.

### The open question is answered: the unrolled chain fits, given egress

`sc_unrolled.p4` is the same NF with the twelve rounds written out linearly instead of as one
re-executed body, split across the two pipelines. **It compiles in 7 s with one recirculation**,
where the rolled ground truth needs two. Ingress uses 19 stages, egress 18, over 139 actions and
40 `@in_hash` sites.

So loop rolling is not required for SmartCookie to fit. Item 1 is a code-size question, not a
feasibility one, and item 2 is both necessary and sufficient. What the failures along the way said:

- **The chain never failed on PHV or hash units once split.** Every failed attempt reported
  "supports up to 20 stages, using 21" (or 25). The hash-unit competition that defeated the
  ingress-only form is gone once half the rounds live in egress, exactly as predicted, and what is
  left is a pure critical-path problem.
- **A SipRound is 4 dependency levels**, so twelve rounds are 48 and a single pass over both
  pipelines (40 stages) can never hold them. Two passes can.
- **Mutually exclusive branches share stages, but only from where they start.** Lap 1's and lap 2's
  code overlay each other in the same stages, so the bound is the largest lap per pipeline rather
  than the sum. But a lap emitted after another lap's tables begins after them: emitting the
  *recirculated* lap first, before the first pass's clock/bloom/triage block, took ingress from 21
  stages to 18, and the same reordering in egress took it from 21 to 18. **The order in which
  alternative code paths are emitted decides whether a program fits.** This is the most directly
  actionable rule for the synthesizer to come out of the experiment, and it is invisible unless you
  read the stage assignment out of the `.bfa`.
- **Ingress is the scarce pipeline**, because the clock, bloom and triage tables sit ahead of the
  chain: it holds about 2 rounds per lap against egress's 4.

**Open, and not understood: the bloom filter stops working in this build.** Everything else passes
`tests/smartcookie.py` against it, including the full SYN to cookie to verified-ACK round trip and
the hash-versus-reference check, so the unrolled twelve rounds are correct. But an ECE packet from
the server no longer records its flow, and the bloom source is byte-identical to the version that
works. The hash inputs of the set and read tables were checked in the `.bfa` and are consistent,
and giving the two read bits their own metadata fields does not help. Whatever it is, it is a third
case of placement alone changing behaviour with no compiler complaint. `sc_unrolled.p4` is kept as
an experiment, deliberately not in `synthesized/`, because it is not a validated solution.

### What item 2 entails

Three things, found by reading the code rather than guessing:

- **The template has nowhere to put egress code.** `tofino.template.p4` has `EGRESS_HEADERS` and
  `EGRESS_METADATA` markers, so the structs can be filled, but the egress parser has no marker,
  `control Egress` is a hardcoded `apply {}`, and the deparser has none. Four or five new
  insertion points are the mechanical part.
- **The placer models one pipeline, not two.** `tna_properties_t` has `stages = 20` and a single
  `PipelineResources`; `pipes = 4` is the physical pipes and is unrelated. A recirculation pass is
  one 20-stage pool. It has to become ingress + egress: two pools with a one-way dependency, and
  anything crossing the boundary has to travel in a header, because the two share no metadata.
- **It is worth more than SmartCookie, but only for the right kind of plan.** Any plan over 20
  stages recirculates today, and recirculation costs throughput. Egress buys **critical-path depth,
  not capacity**: a plan bound by dependency depth, as SmartCookie is, can drop a recirculation,
  while a plan bound by SRAM or logical table ids gains nothing, because those are shared with
  ingress (see below). The search heuristic has to learn that an egress stage is nearly free where
  a recirculation is not, and the placer has to know which of the two kinds of budget is binding.

### How the pipeline's resources actually spread across the two gresses

Measured from `resources.json` and the assembly of the ground truth's own build, because the answer
differs per resource and none of it is half/half.

- **Stage depth: two independent budgets.** Both gresses walk the *same* 20 physical stages;
  `stages = 20` is not 20 ingress plus 20 egress of hardware. What each gress gets on its own is a
  dependency-depth budget of 20, which bf-p4c reports per gress ("using 21"). The unrolled build
  uses 19 ingress and 18 egress and compiles, so the two budgets genuinely do not draw each other
  down. This is the one place egress is free, and it has been entirely unused.
- **Stage memory: one shared pool per physical stage, not a split.** The resource report has 17
  stage entries for a program that uses 17 ingress *and* 17 egress stages, because they are the
  same stages. Ten of the seventeen hold tables from both gresses, drawing on one 80-unit SRAM grid
  (8 rows x 10 columns), 24 TCAM units and 16 logical table ids. Stage 0 holds 2 ingress and 3
  egress tables; stage 12 holds 2 and 2. So `sram_per_stage`, `tcam_per_stage`, `map_ram_per_stage`,
  `max_logical_*`, `hash_dist_units_per_stage` and the xbar figures are per *physical* stage and
  shared. Ingress has had them to itself only because nothing was ever placed in egress.
- **PHV: one pool, partitioned.** Ingress and egress containers are disjoint: 34 and 35 containers
  in the ground truth, with zero overlap. The report's totals (80 8-bit, 120 16-bit, 80 32-bit)
  match `configs/tofino2.toml` exactly, so the `phv_*_containers` numbers are the whole-chip pool
  that gets split. Egress's share is unused today, but claiming it takes it away from ingress.

The model the placer needs is therefore: **two independent depth constraints** (ingress <= 20,
egress <= 20), **one shared memory constraint per physical stage** summing what both gresses put
there, and **one PHV pool partitioned** between them. Today it has a single 20-stage pool with all
of these folded into it, which is right only as long as egress stays empty.

Three constraints bound what can go there:

- **Egress cannot redirect.** The egress port is chosen in ingress; egress can drop but not
  re-route, so any module that changes the forwarding decision stays in ingress.
- **The controller addresses tables as `"Ingress.x"`.** Egress tables would be `"Egress.x"` and
  sycon has no support for that. The ground truth avoids it by using only `const entries` in
  egress, which is a fair restriction to start with but excludes controller-populated tables.
- **The egress parser has to be generated** to match exactly what the ingress deparser emits,
  which today is implicit.
