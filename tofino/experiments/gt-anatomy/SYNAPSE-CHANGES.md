# What synapse has to change to produce the compiling SmartCookie

Target: `./run.sh <tag> 1 2 3 4 H A2 D G W J CS` (+ `CT` on the controller). Compiles for Tofino 2
with 0 errors, 17 ingress / 18 egress stages, 108 tables, egress genuinely live, and it passes the
first case of `tests/smartcookie.py` on the model -- a SYN answered by a correct SYN-ACK whose
cookie the test recomputes and whose checksums it verifies. See README.md and RECIPE.md.

## The headline: the search does not need to change

The plan synapse already finds -- one recirculation, 58 ingress and 22 egress compute actions --
is the plan that compiles. Everything below is in the **emitters**, with two exceptions noted
(items 0 and 5). This was not obvious: for most of this work we assumed the plan was infeasible and
that the search would have to be taught to spend another lap. It is not, and it does not.

Ordering note: items 1, 2 and 9 are correctness bugs that affect **every** NF, are independent of
each other, and should land first. Item 4 unblocks item 3. The rest are PHV-pressure fixes.
Regenerate and diff the six committed NF solutions after each one.

---

## 0. Land the gress-aware placement (written, uncommitted)

`Pipeline.h/.cpp`, `SimplePlacer.cpp`, `SolverPlacer.cpp`, `TofinoModule.cpp`, `Search.cpp`,
`SendToController.cpp`. Stage *depth* becomes per-gress while stage *memory* stays shared, and
`clean_slate_placement` replays each request into the gress it was placed in. Without it the
winner is a 2-recirculation, all-ingress plan; with it, 1 recirculation at the same 286.83 Mpps.

**This is the one change that alters which plan is found.** It needs the six-NF regression sweep
before it lands, because it touches placement for every NF.

## 1. Parser: honour `parser_selection_t::negated`  -- CORRECTNESS, affects every NF

`negated` is set in `Modules/Tofino/ParserCondition.cpp:66` and **never read** in
`Synthesizers/TofinoSynthesizer.cpp`. The emitter always writes `value: on_true; default:
on_false`. BDD node 5 is `!(17 == protocol)`, so the correct emission is the other way round.

Measured consequence: IPv4 protocol 0x11 (UDP) reached `pkt.extract(hdr.hdr2)` (the 160-bit TCP
header) and 0x06 (TCP) fell through to `hdr.hdr3` (64-bit UDP). A TCP packet never got a TCP
header, the entire cookie path was unreachable, and a client SYN was simply routed to the server.

Fix: swap `next_true`/`next_false` when `selection.negated`, around
`TofinoSynthesizer.cpp:2494`.

## 2. Egress parser must extract every header the crossing can emit -- CORRECTNESS

`egress_parser_hdrs` (`TofinoSynthesizer.cpp:2835`) is built only from the `ParserExtraction`
modules seen past the cut. But on the crossing path the ingress also validates `hdr.recirc` (via
`build_recirc_hdr`) and the deparser emits it, and `recirc_h` precedes `egress_state_h` in the
header struct -- so the egress parser was reading `egress_state` out of the recirculation header's
bytes, 40 bytes off, and `hdr.recirc` was invalid in egress.

Measured consequence: the egress's eight writes to `hdr.recirc.f32_*` went nowhere and bf-p4c
eliminated the **entire 22-action egress chain** as dead code (84 tables, "stages for egress table
allocation: 1"). The 1-recirculation plan was therefore *incorrect*, and the crossing looked free
to the search because it did nothing.

Fix: include every header the ingress deparser can emit on that path, in struct order.

## 3. Deparser checksums -- MISSING FEATURE

Synapse emits a bare `pkt.emit(hdr)`. The ground truth recomputes the IPv4 and TCP checksums.
Without this the crafted SYN-ACK is byte-identical to the expected packet except for two checksum
bytes.

Needs: `Checksum()` externs in the deparser; an update per checksum whose field list is the
covered header minus the checksum field itself; a guard so only rewritten packets are recomputed
(a pass-through packet's payload is not visible to the deparser); and the TCP pseudo-header length
staged through metadata, because bf-p4c rejects literals in a checksum list ("Non-zero constant
entry in checksum calculation not implemented yet"). The ground truth carries `eg_md.tcp_len` for
exactly this reason.

## 4. Header field coalescing must respect whole-value reads, and expose checksum fields

Two independent failures come from the same place -- the guessed field boundaries in
`Modules/Tofino/ParserExtraction.cpp` (`hdr_fields_guess`) and the coalescing in
`TofinoSynthesizer::visit(ParserExtraction)`:

- the 32-bit TCP ack straddled a guessed 24/24 boundary, so `compose_hdr_fields` emitted
  `hdr.hdr2.data2 ++ hdr.hdr2.data3[23:16]`, and bf-p4c rejects a concat as an ALU operand
  ("read in a way too complex for the compiler to currently handle");
- the IPv4 and TCP checksums sit inside wider fields (`hdr1.data2[15:0]`, `hdr2.data4[31:16]`),
  which blocks item 3 -- a `Checksum()` result cannot be assigned to a slice.

Rule: a byte range that is read or written as a unit anywhere should be its own field.

## 5. Classify byte-aligned concat rotates as Hash ops

`Modules/Tofino/RotateLeft.cpp:47`: `kind = (amount % 8 != 0) ? Hash : ALU`. A byte-aligned concat
rotate is *legal* bare, but it still **cuts its operand's container**, and the cut propagates
through the xor/add chain. Wrapping all of them took the program from 116 unallocated slices to 84.

Changing the classification does three things with one edit: the emitter wraps them in `@in_hash`,
the existing `MAX_HASH_BITS_PER_ACTION` budget then keeps one per action by itself (bf-p4c allows
32 bits through a table's immediate pathway; two 32-bit hash ops in one table is an error), and the
placer charges the hash-distribution units.

**This one feeds back into the search** -- it changes op kinds, the per-action hash budget and the
per-stage hash-dist charge, so the chosen plan may move. Re-measure after landing.

## 6. Write compute values into deparsed headers through the hash unit

Covers hand fixes A2 (`hdr.cpu.*`), D (`hdr.hdr2.*`) and W (`hdr.egress_state.*`, `hdr.recirc.*`).

`hdr.cpu.X = meta.X` copies a metadata field the rotate chain has cut into a `deparsed
exact_containers` field. The header field cannot be split, so every boundary piece of the source
becomes its own PHV source -- eight of them in one measured case, against a limit of two. The
ground truth never does this as a plain ALU copy; it writes such fields as
`@in_hash { hdr.hdr2.data2 = ctime ^ v0 ^ v1 ^ v2 ^ v3; }`.

Restrict it to sources the chain actually produces: wrapping `hdr.recirc.ingress_port =
meta.ingress_port` put a second `@in_hash` into an action that also takes action data, which is 48
bits through the immediate pathway and needs the hit pathway.

Synapse's carried state is the thing under pressure here -- 12 32-bit fields in `cpu_h`, 14 in
`egress_state_h`, 9 in `recirc_h` against the ground truth's 0 + 5 + 1. Reducing it would help
independently, but routing the writes through the hash unit is what made it fit.

## 7. Emit operations at the natural width of their operands

`((bit<32>)(hdr.hdr2.data3[7:0])) | 32w0x12` violates action constraints; `hdr.hdr2.data3[7:0] |
8w0x12` does not. The BDD works in 32-bit ints, but when the result is only ever read at 8 bits the
operation should stay 8 bits. The ground truth writes `hdr.hdr2.data4[7:0] | 8w0x12`.

## 8. Peephole: an identity byte-concat is a copy

`x[31:24] ++ x[23:16] ++ x[15:8] ++ x[7:0]` is `x`, but bf-p4c reads it as four PHV sources and
rejects the action. Emitted by the byte-swap path in `visit(ModifyHeader)` when the permutation
happens to be the identity.

## 9. Controller: do not register dataplane tables for offloaded nodes -- CORRECTNESS

`ControllerSynthesizer` emitted
`VectorTable(..., {"Ingress.vector_table_..._105", "..._88" x4})`, but node 88's read was offloaded
to the controller so its dataplane table is never emitted. libsycon aborts in `build_table`
("Object not found") and the controller never starts. The duplicates are wrong on their own too:
`VectorTable` sums `value_size` across the list, so replicas must not repeat.

## 10. OPEN -- the offloaded node's controller replay produces nothing

Model-test case 2 (a valid cookie reaching the server, ECE-tagged, seq - 1) gets no packet. The
dataplane does the right thing -- bloom miss, cookie-verification path, SipHash started -- and then
hands the packet to the controller (`tbl_build_cpu_hdr`, `ucast_egress_port = 0`), which logs one
`RX` and emits nothing. Not yet diagnosed. Note the P4 contains no ECE/seq-1 write at all, which is
consistent with that whole branch living in the controller.

---

## Known-wrong in the resource model, but not needed for this solution

- `Pipeline::get_used_stages()` counts one shared stage vector; bf-p4c budgets 20 stages **per
  gress**. The winner scored "Stages: 20" while bf-p4c reported 17 ingress + 18 egress.
- Nothing models hash-distribution units per stage. bf-p4c fits exactly three 32-bit `@in_hash`
  tables per stage (6 units, 2 each), which is visible as a run of 3-tables-per-stage rows in
  `table_summary.log`.

Both matter for other NFs and for robustness once item 5 raises hash usage, but the plan that
compiles fits without either.

## Falsified along the way -- do not re-attempt

- Liveness slot pooling of the chain (191 -> 23, cut-blind or cut-signature aware): **worse**
  (272-360 unallocated). A pooled slot accumulates the boundaries of every value that passes
  through it, and cuts unify across the chain anyway because `a = b ^ c` forces b and c into
  compatible layouts.
- Moving the pooled chain values into a header: worse (304, then 360).
- `@pa_container_size` on the chain: the pragma marks a field `solitary`, so 19 pins consume 19
  32-bit containers exclusively and starve `hdr.hdr2.data2` and `meta.time`. The ground truth's own
  16 pragmas are **not** essential -- removing them still compiles with a byte-identical
  allocation, only slower (9.5 s -> 32.6 s).
- Shift-form rotates for live-out values: worse (102 vs 66).
- One `@in_hash` per action, or moving ALU reads of a hash-rotated value into their own action, as
  a way to stop a cut spreading: no effect at all (84 -> 84).
