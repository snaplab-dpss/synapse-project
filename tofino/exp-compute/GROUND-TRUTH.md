# Ground-truth SmartCookie

Two complete SmartCookies for Tofino 2, written by hand, that compile **and pass the model test**
(`tests/smartcookie.py`). They are the reference the synthesized solution should be judged against:
what synapse ought to produce, and the yardstick for how far its output is from something the
target accepts.

| | `smartcookie-unrolled.p4` | `smartcookie-manual.p4` |
|---|---|---|
| the twelve rounds | written out linearly | one body, re-executed |
| rounds per lap | 2 ingress + 4 egress | 2 ingress + 2 egress |
| laps / recirculations | 2 / **1** | 3 / 2 |
| stages | 18 ingress, 19 egress | 17 ingress, 17 egress |
| match tables | 82 | 47 |
| `@in_hash` sites | 40 | 8 |
| compile time | 7 s | 6 s |

**`smartcookie-unrolled.p4` is the one synapse should aim at.** Separate code per lap is what
synapse's BDD already produces, so it needs no loop rolling, and it costs one recirculation instead
of two. The rolled one is kept because it is the smaller, clearer program, and because it is where
most of the target rules below were learned.

Both live in `synthesized/` rather than here so they sit next to the machine-generated solutions
they are meant to be diffed against, and so the testbed can run them like any other NF:

```
bf-p4c --target tofino2 --arch t2na -o out synthesized/smartcookie-unrolled.p4   # 7 s
sudo -E tests/testbed.py up smartcookie-unrolled
sudo -E python3 tests/smartcookie.py --nf smartcookie-unrolled
```

Both follow synapse's own P4 template: the same `synapse_ingress_headers_t` and
`synapse_ingress_metadata_t` structs, the `cpu_h` / `recirc_h` / `recirc_state_h` headers, the
`hdr0..hdr4` layout that synapse's header guessing produces for ethernet, IPv4, TCP and UDP, the
`TofinoIngressParser` and `IngressParser` shape, the `fwd_op_t` enum, `ingress_port_to_nf_dev` and
the usual forwarding actions. A reader should be able to diff it against a synthesized file.

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

That is `smartcookie-manual.p4`. `smartcookie-unrolled.p4` keeps the same semantics and the same
recirculated state, but writes the rounds out one after another, two in ingress and four in egress
per lap, over two laps; its message words are inlined rather than chosen by a table, and the lap is
identified by `hdr.recirc.code_path`, which is what synapse already emits.

Each element of both was forced by a measured failure. See `README.md` for the experiments behind
them, and the section at the end for what the unrolled one settled.

## The work: first, make it compile

Every item below started as "synapse cannot express this". Testing each one before planning any
work shrank most of them: one dissolved entirely, one left the critical path, and two turned out to
be largely implemented already. What follows is ordered by the order we will actually take, with
the original numbering kept in brackets so older notes still line up.

**Items 1 to 3 are what it takes to make synapse emit a SmartCookie that compiles**, which is the
milestone we are aiming at, and they are the only ones on the critical path. Items 4 to 6 are
correctness and quality: none of them stops a program compiling, and checksums in particular mean
the result will not be *correct* until item 4 lands, and item 5 guards against state being
silently duplicated.

The five rules under "Emission rules the compiler will not enforce" are not separate work; each
attaches to whichever item touches it.

### 1. Use the egress pipeline *(was item 2)*

All synapse output lives in ingress. The ground truth needs egress for half its rounds, and the
unrolled experiment showed this is the whole of what SmartCookie needs. It also pays off beyond
SmartCookie: any plan over 20 stages recirculates today, and recirculation costs throughput.

But egress buys **critical-path depth, not capacity**. A plan bound by dependency depth, as
SmartCookie is, can drop a recirculation; a plan bound by SRAM or logical table ids gains nothing,
because those are shared with ingress. The search heuristic has to learn that an egress stage is
nearly free where a recirculation is not, and the placer has to know which budget is binding.

**The template has nowhere to put egress code.** `tofino.template.p4` has `EGRESS_HEADERS` and
`EGRESS_METADATA` markers, so the structs can be filled, but the egress parser has no marker,
`control Egress` is a hardcoded `apply {}`, and the deparser has none. Four or five new insertion
points are the mechanical part.

**Crossing into egress is its own module, `SendToEgress`.** Like `Recirculate` it sits at a point
in the plan and means "everything after here runs in the egress pipeline", so the split is a single
cut rather than a per-operation choice: one decision per pass, and a clean invariant that makes the
header-crossing reuse the machinery `visit(Recirculate)` already has for moving the live set into a
header. Its guard is that **no port-selecting decision may remain downstream**, which forces the
search to consider egress only when the routing decision can be pulled back ahead of the cut. Two
details the guard has to get right:

- **Dropping in egress is still allowed**, only choosing a port is not. The ground truth depends on
  this: the `cookie_age` check drops aged cookies from egress.
- **Deciding to recirculate is a port selection**, since recirculation is `ucast_egress_port` set to
  the recirculation port in ingress. So it too must be resolved before the cut.

It also has to be costed at nearly nothing, unlike a recirculation: it is the same packet on the
same pass. If the oracle does not say so, the search will never choose it.

**The placer models one pipeline, not two.** `tna_properties_t` has `stages = 20` and a single
`PipelineResources`; `pipes = 4` is the physical pipes and is unrelated. A recirculation pass is one
20-stage pool. It has to become ingress + egress, and anything crossing the boundary has to travel
in a header, because the two gresses share no metadata.

**Absorbed from the dissolved item 3:** the egress parser has to extract the recirculation header
and the egress control has to open with the same `if (hdr.recirc.code_path == N)` chain the ingress
has. Synapse already emits that chain in ingress and already carries live variables across a
recirculation, so this is reuse, not new machinery.

Three constraints bound what can go in egress:

- **Egress cannot redirect.** The egress port is chosen in ingress; egress can drop but not
  re-route, so any module that changes the forwarding decision stays in ingress.
- **The controller addresses tables as `"Ingress.x"`.** Egress tables would be `"Egress.x"` and
  sycon has no support for that. The ground truth avoids it by using only `const entries` in
  egress, which is a fair restriction to start with but excludes controller-populated tables.
- **The egress parser has to be generated** to match exactly what the ingress deparser emits, which
  today is implicit.

#### What implementing it turned up (2026-09-09)

- **The obvious guard cannot fire.** "Cross only once the forwarding decision has been made" never
  triggers, because every BDD path ends in its own route node: by the time a decision exists there
  is nothing left to do. `SendToEgress` never appeared in the search space at all. The guard has to
  *pull the decision back* instead: allow the cut when every route still reachable agrees on the
  device, and have the module emit that decision itself. Drops downstream are fine, broadcast is
  not. With that, the module appears 122 times in the search space.
- **Past the cut, `Forward` emits nothing and `Drop` uses the egress drop control.** The port was
  written before the crossing, so the route node in egress has nothing left to do.
- **Offering the crossing everywhere destroys the search.** The estimated search space went from
  2.0e11 to 2.3e61 and the winner got *worse* (259 Mpps, 3 recirculations, against 287 and 2),
  because the option is pure noise until the placer can exploit it. Requiring that the pass already
  holds real work before the crossing is offered brings it back to 5.6e12.
- **The depth budget is a two-line change.** `collect_deps_from` and `was_ds_already_used` already
  stop walking at a recirculation; stopping at an egress crossing too gives the egress its own
  dependency chain. Stage memory deliberately keeps accumulating, which is exactly right: the two
  gresses share the physical stages.

#### Where the implementation actually stands (2026-09-09, overnight)

Built and working, on main, every commit building:

- The template has egress insertion points, a generated `egress_state_h` for values crossing the
  boundary, and a synthesizer-written bypass decision.
- A `SendToEgress` module, guarded on every reachable route agreeing, emitting the pulled-back
  forwarding decision itself.
- The egress parser is generated from the headers extracted on the path to the cut.
- Metadata past the cut is named `eg_md`, `Forward` becomes a no-op there and `Drop` uses the
  egress drop control.
- Crossing starts a fresh dependency chain, so the egress gets its own depth budget.

**Synapse does emit egress code.** One run put the whole HalfSipHash round body in the `Egress`
control, reading its inputs from `hdr.egress_state`, with a matching egress parser. So the
machinery works end to end.

**The blocker is a dead end, and it is not the crossing's fault.** Widening which BDD calls the
crossing will accept makes the search die with "Dead end reached! No module can handle this BDD
node". The obvious reading is that the crossing was declined where it was the only way on, so it
was tested directly: with the guard **fully open** -- every call accepted, the "pass has real work"
threshold at zero, the forwarding-decision exemption in place -- the search still dies at exactly
the same step, 113. The dumped plan contains no crossing at all, and instrumentation shows
`SendToEgress` is never even consulted at that leaf.

So this is a **pre-existing fragility that different exploration exposes**, not something the
crossing causes: a plan where BDD reordering hoisted the route, `Forward` was placed and work was
left over, which no module can continue. Merely registering a new factory changes the order the
search walks in and steers it there. (Note synapse already ships `--allow-deadends` for heuristics
that hit this, which we do not use.)

Numbers from one run before that was understood, still useful: of 178 offers, 172 were declined by
the implementability allowlist, 0 by the uniform-route test and 0 by the "pass has real work" test.
The four call names it declines are `bf_query`, `vector_borrow`, `vector_return` and
`nf_set_rte_ipv4_udptcp_checksum`.

**Next step: find out why no module can handle that node.** The instrumentation to write is in the
search's dead-end path, not in the module -- dump which factories were asked and why each declined.
Until then the allowlist is deliberately narrow, which keeps the search on paths it can finish: it
completes in 468 steps with no dead end and produces the same solution as before, so main is in a
working state with the egress path dormant rather than half-wired.

Done since, and safe on its own: data structures declared past a crossing are addressed as
`Egress.<id>` in the generated controller. libsycon needed no change at all -- its `Table` takes the
name as a plain string -- and only `ControllerSynthesizer.cpp` hardcoded `"Ingress."`, in 17 places.
`cl`'s controller comes out byte-identical.

Verified after the change: `cl` regenerates structurally identical (only EP node ids renumber,
because registering a new factory shifts them) and compiles with 0 errors.

#### How the pipeline's resources actually spread across the two gresses

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

### 2. Choose the order alternative code paths are emitted

Split out of item 1 because it is required to compile, not an optimisation, and it is invisible
unless you read the stage assignment out of the `.bfa`.

Mutually exclusive branches share stages, but only from where they start: a lap emitted after
another lap's tables begins after them. Emitting the *recirculated* lap first, ahead of the first
pass's clock/bloom/triage block, took ingress from 21 stages to 18, and the same reordering in
egress took it from 21 to 18 and made the program compile. So the synthesizer has to choose the
order it emits alternative code paths in, and the placer has to understand that choice.

### 3. `@in_hash` for an expression that would span stages *(was item 4)*

Synapse already has half of this: `RotateLeft::uses_hash_unit()` sends any rotate that is not a
whole number of bytes through `@in_hash`, and `ComputeAction` budgets hash bits and hash
distribution units. What is missing is the general case: **an expression tree that would span stages
collapses into one hash-unit op.** `v0 ^ v1 ^ v2 ^ v3` costs three ALU instructions and an action
cannot span stages ("xor: action spanning multiple stages"), but it is a single `@in_hash` op.

**Verified, and the reason is not what it looks like.** Synapse's arithmetic unrolling
(`LibBDD/Unroll.cpp`) already turns every multi-operand expression into one `op_*` BDD node per
operation, `Xor` included, and hyperloglog's shipped P4 shows `op_sub` then `op_add` as separate
nodes. So synapse would emit three chained xors, not one illegal action, and this looked like a mere
stage-budget cost. It is not. Four variants of `smartcookie-unrolled.p4`, changing only how the final xor is
written:

| how the 4-way xor is emitted | result |
|---|---|
| one `@in_hash` op | compiles |
| three chained **ALU** xors into three metadata fields | PHV allocation fails |
| three chained **ALU** xors into one metadata field | PHV allocation fails |
| three chained xors, each through **`@in_hash`** | compiles |

The failure names `hdr.recirc_state.v2` fragmenting into bit slices, the same failure mode as the
original ingress-only SmartCookie. So the rule is not "collapse an expression into one op" but
**operations on the hash state words have to go through the hash unit**: an ALU read of `v0..v3`
adds operand-alignment constraints to containers that are already carrying many, and the allocator
fragments them; a hash-unit read imposes none. Collapsing into a single op is one way to satisfy
that, three hash ops are another. **This item is required to compile.**

Two other uses in an earlier version of this file turned out to be superstition: a slice-and-widen
read of an intrinsic (`ctime = ingress_mac_tstamp[47:16]`) and a byte read feeding a table parameter
(`nf_dev = dst[31:24]`) both compile as plain ALU ops and pass the model test. They have been
removed, taking the program from ten `@in_hash` sites to eight.

## Then: correctness, quality, and what is optional

### 4. Deparser checksums *(was item 7)*

Behind the compile-critical items only because it does not stop a program compiling. It is
otherwise the most urgent thing here: not a SmartCookie feature but a **live correctness bug in
shipped solutions**, and independent of everything else on this list, so it can proceed in
parallel.

Synapse emits no dataplane checksum at all: `Ignore.cpp` lists `nf_set_rte_ipv4_udptcp_checksum`
among the calls it drops, and `ModifyHeader.cpp`'s `filter_out_checksum_mods` strips the checksum
field write from the header modifications. So a packet the *dataplane* rewrites leaves with the
checksum it arrived with; only packets that take the CPU path are corrected, by the controller's
`update_ipv4_tcpudp_checksums`.

Verified on a shipped solution: `nat-f40000-c0-unif-hmax-tput` with `NAT_INDEX_BYTE_ORDER=big`
fails `tests/nat.py` on the fast path with both the IPv4 and the UDP checksum wrong, and passes as
soon as `NAT_CHECK_CHECKSUMS=0` is set. Checksums are the only thing still failing there.

The target's rule, measured both ways and asymmetric in a way that matters:

- a `Checksum()` **input** that is a slice is a hard error, "unexpected type of parameter
  hdr.hdr1.data2[39:8] in Checksum";
- a `Checksum()` **output** written to a slice compiles with 0 errors and is **silently ignored**,
  leaving the original checksum on the wire. This is how the first version of this program shipped a
  wrong checksum past the compiler.

So every field a checksum touches must be a header field in its own right, which is a constraint on
**header guessing**, not just on emission: the IPv4 protocol and header checksum are separate fields
here, as are the TCP window, checksum and urgent pointer. The ground truth shows the whole shape of
the fix, a deparser `Checksum().update({...})` guarded by a flag the rewriting actions set.

### 5. A register's read and write must share one table *(new)*

Found by the unrolled experiment, and the worst silent failure of the investigation: **bf-p4c will
split a `Register` across two stages rather than refuse.** `Ingress.bf_row_0` was allocated SRAM in
stages 4 *and* 5, with a stateful ALU in each (`tbl_bf_add$salu`, `tbl_bf_query$st1$salu`), so the
bloom's write and its read landed on different copies of the state and never saw each other. The
program compiled with 0 errors and quietly lost every insertion.

The fix is an emission rule: **put a register's read and its write in the same table**, as actions
selected by a key, so they cannot be placed in different stages. That put each row back into one
stage and the unrolled solution passes.

Synapse today does the opposite: PSD emits `..._read_and_set`, `..._set_to_one` and `..._read` as
three separate actions per bloom row, each called bare from the apply block and so each its own
keyless table. Checked across the shipped solutions and **no register is currently split** (psd 5
registers, cl 8, nat 0, each in one stage), so this is latent. But it is triggered by placement
pressure, and item 1 exists precisely to let synapse build denser programs, so the risk goes up as
soon as that lands.

### 6. The immediate pathway is a per-table budget *(was item 5)*

Not "one hash-producing action per table": the limit is **32 bits of hash-produced immediate data
per table, summed over its actions**. Merging the two final-xor actions into one table gives "the
number of bits required to go through the immediate pathway 64 ... is greater than the available
bits 32", and bf-p4c then crashes with SIGSEGV rather than exiting cleanly.

Synapse models this per *action* (`ComputeAction::MAX_HASH_BITS_PER_ACTION`), which agrees with the
hardware today only because it emits one action per keyless table. The accounting has to move to the
table as soon as one table carries several actions.

The claim that a hash operation cannot sit in a keyless table is **wrong**: every round action here
holds an `@in_hash` and is called bare from the apply block, and bf-p4c compiles each into a keyless
`hash_action` table. The bloom's `Hash.get()` queries are the same shape.

### 7. Gateway comparisons *(was item 6)*

Measured, the gateway budget splits by comparison kind rather than by width alone:

| condition | limit on PHV operands |
|---|---|
| `==`, `!=` | 4 bytes, so a 32-bit equality fits |
| `<`, `>`, `<=`, `>=` against a power-of-two boundary | free at any width: it is a "high bits are zero" mask test |
| `<`, `>`, `<=`, `>=` otherwise | **12 bits total**, and a constant operand costs nothing |

Toys: `cmpk12.p4` (12-bit field vs a constant) compiles, `cmpk13.p4` (13 bits) does not, and
`cmp12.p4` -- two 12-bit *fields*, 24 bits of operand -- does not either. `age > 2` on a 32-bit
value is why the three accepted epochs are constant table entries in the ground truth.

Synapse already covers most of this. `If.cpp`'s `is_wide_const_inequality` diverts a relational
against a constant wider than 8 bits away from the gateway, exempts the power-of-two case, and falls
back to rewriting the comparison over narrow slices rather than punting to the controller. Two gaps
remain, both latent:

- the threshold is 8 bits where the hardware allows 12, so some conditions are split that need not
  be;
- a relational between **two non-constant operands** is not checked at all: it bypasses
  `is_wide_const_inequality` (which requires a constant RHS) and passes `condition_meets_phv_limit`,
  which counts bytes against 4 without regard to comparison kind. Two 12-bit fields are 2 bytes and
  sail through, and bf-p4c then rejects the program. The check needs to sum the non-constant operand
  widths against 12 bits for relational comparisons.

### Not planned: emit one body executed many times *(was item 1)*

The rolled ground truth writes the chain once and re-executes it each lap, so three laps cost one
lap of hardware, and this was originally the headline gap: synapse emits separate code per lap
because its BDD arrived from symbolic execution with the loop already unrolled.

**The measurement removed the reason to do it.** The unrolled solution needs *one* recirculation
where the rolled one needs two, so rolling would not buy throughput; it would cost one. What it
would buy is code size, 142 actions against 67. Against that it needs the repeated structure
recognised in the BDD, a body emitted once, and the search and placer taught what that costs, which
is by far the largest piece of work on any version of this list.

Kept as a note rather than an item: nothing in SmartCookie needs it. If another NF turns out to be
bound by code size rather than by recirculations, revisit.

### Dissolved: a loop counter and dispatch on it *(was item 3)*

Only a *rolled* loop needs a runtime counter (`round = round + 2` with tables keyed on it). The
unrolled form needs a **pass identifier**, which is a plan-time constant, and synapse already emits
one: `build_recirc_hdr(N)` writes `hdr.recirc.code_path` and an `if (hdr.recirc.code_path == N)`
chain dispatches on it. `smartcookie-unrolled.p4` was written to use exactly that, with no counter
field of its own, and it compiles in 7 s, dispatching in egress as well as ingress. Nothing to build.

## Emission rules the compiler will not enforce

An earlier version of this program compiled in 4 s with 0 errors and was wrong in six ways. Five are
constraints the emitter has to respect; they are listed here because a synthesizer that only checks
"does bf-p4c accept it" will violate every one of them and never find out. The sixth was a plain
transcription slip on my part, hashing `ack` where the cookie check wants `ack - 1`, which is the
other reason `tests/smartcookie.py` exists.

1. **Statements inside an action run in order.** A two-field swap written as `a = b; b = a;`
   duplicates `b`. The old value has to be captured by an earlier action, because a temporary
   written and read inside one action makes it span stages, which bf-p4c does reject. A *single*
   whole-field write of a field in terms of itself is fine and is one operation:
   `ports = ports[15:0] ++ ports[31:16]` swaps correctly.
2. **The same ordering across tables.** `ack = seq + 1` has to read `seq` before the table that
   writes the cookie into it, so it is staged on the way into the pipeline.
3. **A deparser `Checksum()` can neither read nor write a slice**, and the write is silently
   ignored. See the work item above; staging inputs into metadata fixes the read side and does
   nothing for the write side, which is why header fields have to be split instead.
4. **A recirculated packet still has to parse.** Marking a packet in flight by rewriting its
   ethertype made the ingress parser reject it on the way back round, so the second and third laps
   hashed nothing. The recirculation header's `code_path` is the marker; the ethertype is left alone.
5. **An action that uses the hash distribution unit cannot be a table's `default_action`**
   ("Cannot specify bf_query_0 as the default action, as it requires the hash distribution unit").
   It can be a `const entries` action, and it can sit in a keyless table called bare from the apply
   block. This is very likely the origin of the old, wrongly generalised note that a hash operation
   cannot sit in a keyless table at all.
6. **`f = C ++ (f[7:0] | K)` silently loses the OR.** This one is a bf-p4c bug, not a rule to
   follow: it allocates a temporary for the concat operand and never writes it
   (`set hdr.f.0-7, $concat_to_slice27`, with nothing anywhere assigning `$concat_to_slice27`), so
   the field comes out zero. Two slice assignments compile to `set hdr.f.8-15, C` plus
   `or B7, K, B7` and are correct. Reproducers: `concatE.p4` (broken), `concatF.p4` (correct).

Rules 1, 2, 4 and 6 appear in no solution synapse ships today: a scan of `synthesized/*.p4` finds no
in-action swap, no concat containing an operation and no ethertype rewrite. They become live the
moment the emitter is taught to rewrite packets in place. Rule 3 is already live, as work item 2
says.

## What the unrolled solution established

`synthesized/smartcookie-unrolled.p4` is the same NF with the twelve rounds written out linearly
instead of as one re-executed body, split across the two pipelines. **It compiles in 7 s with one
recirculation and passes `tests/smartcookie.py`**, where the rolled ground truth needs two
recirculations. Ingress uses 18 stages, egress 19, over 142 actions.

- **The chain never failed on PHV or hash units once split.** Every failed attempt reported
  "supports up to 20 stages, using 21" (or 25). The hash-unit competition that defeated the
  ingress-only form is gone once half the rounds live in egress, and what is left is a pure
  critical-path problem.
- **A SipRound is 4 dependency levels**, so twelve rounds are 48 and a single pass over both
  pipelines (40 stages) can never hold them. Two passes can.
- **Ingress is the scarce pipeline**, because the clock, bloom and triage tables sit ahead of the
  chain: it holds about 2 rounds per lap against egress's 4.

**It passes `tests/smartcookie.py`.** The bloom filter did stop working at first, with source
byte-identical to the version that works, and the cause turned out to be worth more than the
experiment: **bf-p4c split a `Register` across two stages.** `Ingress.bf_row_0` was allocated SRAM
in stages 4 *and* 5, with two stateful ALUs, `tbl_bf_add$salu` in one and `tbl_bf_query$st1$salu` in
the other. The ECE packet set the bit in one copy and the client packet read the other. `bf_row_1`,
placed in a single stage, was fine. 0 errors reported.

The fix is an emission rule, and it is now work item 5: **put a register's read and its write in the
same table**, as actions selected by a key, so they cannot be placed in different stages. Doing that
put each row back in one stage and the test passes, three runs in a row.

That change surfaced one more target rule: **an action that uses the hash distribution unit cannot
be a table's `default_action`** ("Cannot specify bf_query_0 as the default action, as it requires
the hash distribution unit"). Both bloom actions became `const entries` with a `nop` default. This
is very likely the origin of the old, wrongly generalised note that a hash operation cannot sit in a
keyless table.
