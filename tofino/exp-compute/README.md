# Toy P4 experiments for bf-p4c (Tofino 2, SDE 9.13.4)

Small programs compiled to learn how bf-p4c reacts to a code shape before synapse relies on
it. Each file is one question; the answer is what the compiler did. Compile with the SDE's
compiler:

```
bf-p4c --target tofino2 --arch t2na -o out_X X.p4
```

Build directories (`out_*`) and logs are not kept. A healthy program here compiles in seconds;
anything still running after a couple of minutes is failing slowly and should be treated as a
failure.

`GROUND-TRUTH.md` describes the two hand-written SmartCookies that compile and pass
`tests/smartcookie.py` -- `synthesized/smartcookie-unrolled.p4`, the one synapse should aim at, and
`synthesized/smartcookie-manual.p4`, the smaller rolled sibling -- and lists the work that would let
synapse produce them. That file is the point of everything below.

## Skeleton

`cmp12.p4`: ethernet + ipv4 parser, a few metadata fields, an `init` action and a forwarding
decision. Every toy is this file with the actions replaced. Note that the skeleton as written does
**not** compile: its `meta.t0 <= meta.t1` on two 12-bit fields is 24 bits of gateway operand. The
toys derived from it replace that condition, which is why they build.

## Gateway comparisons (`cmp12.p4`, `cmpk12.p4`, `cmpk13.p4`)

Question: how wide a comparison fits a gateway? The error names the budget, "limit of 4 bytes +
12 bits of PHV input", and the two halves serve different comparisons:

| condition | limit on PHV operands |
|---|---|
| `==`, `!=` | 4 bytes: a 32-bit equality fits |
| `<`, `>`, `<=`, `>=` against a power-of-two boundary | free at any width, it is a mask test on the high bits |
| `<`, `>`, `<=`, `>=` otherwise | 12 bits total; a constant operand costs nothing |

`cmpk12.p4` (12-bit field vs a constant) compiles; `cmpk13.p4` (13 bits) does not; `cmp12.p4`
(two 12-bit fields, 24 bits) does not. A 32-bit `==` compiles, a 32-bit `>` does not, and a 32-bit
`< 4` does -- which is why `kvs`'s shipped `ts_1_diff < 16384` is legal.

## Header-field concatenation (`concat*.p4`)

Question: synapse's header guess splits the IPv4 source address into 24 + 8 bits, while the
NF hashes it as one 32-bit network-order word. Can the P4 rebuild it with `++`?

- `concatA`: `meta.v = hdr.src_hi ++ hdr.src_lo` as a bare assignment: compiles.
- `concatB`: the same concat used directly as an xor operand: PHV allocation fails.
- `concatC`: byte-reversed concat of slices (the bswap32 macro spelled out): compiles.
- `concatD`: slicing a parenthesised concat, `(a ++ b)[7:0] ++ ...`: compiles.
- `concatE`: `f = C ++ (f[7:0] | K)`: compiles, and is **silently wrong**. The assembly reads
  `set hdr.ipv4.frag.0-7, $concat_to_slice1` with nothing anywhere assigning that temporary, so
  the low byte comes out zero and the OR is lost. Found by the model test, not the compiler.
- `concatF`: the same write split into `f[15:8] = C; f[7:0] = f[7:0] | K;`: correct, and the
  assembly shows the `or` instruction.

Takeaway: a field concat must be materialised by its own assignment before any arithmetic
uses it; the existing `bswap32` macro works over a concat; and an operation inside a concat that
is written straight back to the field is dropped, so write the pieces separately.

## Hash rotates, ALU chains and headers (`phvT*.p4`, `phvW*.p4`)

Question: the synthesized SmartCookie fails PHV allocation around its `@in_hash` rotates.
Which shape triggers it?

- `phvT1..T4`: odd-bit `@in_hash` rotates on metadata chained through xor/add, results and
  inputs copied into a recirculation-style header: all compile.
- `phvW1..W3`: the chain's final value written byte-wise into a 40-bit header field: fail.
  `phvW5`: the same writes from a plain value: compiles. `phvW4`: the chain's value written
  whole into a 32-bit `hdr.tcp.seq`: fails.
- `phvW6..W8`: W4 without the `>> 12`, or with byte shifts: still fail. `phvW9`: the shifted
  value xored with a plain value instead of the chain: compiles.
- `phvW10`: hash inputs read from a dedicated header: fails. `phvW11`: the rotate input in its
  own variable: fails. `phvW12`: both rotates in shift form: compiles.

Takeaway (from bf-p4c's own message on W4): the allocator slices the ALU group at the rotate
input's odd bit, and a 32-bit add on operands sliced like that needs four PHV sources where
Tofino 2 allows two.

This is the finding that sent us down the shift-form path, and it was the wrong lesson to draw.
The expert agent rotates the same way and compiles, because its odd rotates go through
`@in_hash` on values that live in their own pinned containers. What actually breaks is a
*shared* container collecting a slice boundary for every rotate amount that ever passes through
it. See `GROUND-TRUTH.md`.

## Actions and stages (`arxR*.p4`)

`arxR<R>L<L>.p4` is the skeleton plus a synthetic add-rotate-xor chain of R rounds, emitted the
way synapse emits: statements grouped into named actions, each called once from the apply block.

| file | chain actions | result |
|---|---|---|
| `arxR4L1` | 12 | compiles |
| `arxR12L1` | 36 | "tofino2 supports up to 20 stages, using 37" |
| `arxR24L1` | 72 | "supports up to 20 stages, using 73" |

Takeaway: **dependent** actions each need their own stage. In this toy every round feeds the
next, so 36 chain actions need 37 stages. Independent actions do share a stage (the expert puts
11 tables in stage 0), so this is a critical-path bound, not a per-action cost.

## The synthesized SmartCookie, and why it does not compile (`sc/`)

- `sc/sc.p4`: the synthesized solution as it first stood. Fails PHV allocation.
- `sc/sc_r21.p4`: synapse's output after the emission rules below were added to the synthesizer.
  Still fails, and its recirculation passes are emitted **nested**
  (`if (code_path==0) { ... if (code_path==1) { ... } }`) with nothing assigning `code_path`
  between them, so every pass after the first is unreachable. That is a correctness bug in
  `TofinoSynthesizer::visit(Recirculate)`, not only a compile problem, and it affects 14 of the
  committed solutions across `cl`, `nat`, `psd` and `fw`.
- `sc/sc_m1.p4`: the same program with the passes rewritten as an if / else-if chain, which is
  what they should have been.
- `sc/sc_x0.p4`: the whole program with the hash chain deleted and nothing else touched.
  **Compiles in 15 s.** So the tables, bloom filter, keys, recirculation and controller headers
  and forwarding are all fine; the chain is the entire problem.

## Where the chain actually fails (`sc/sc_k*.p4`, `sipmin*.p4`, `sipfull.p4`)

`sipmin.p4` is the skeleton plus the expert's chain idiom: four state words in a header, four
pinned metadata temporaries, eight round actions, two rounds. **Compiles in 10 s, and in 2 s
with the container pins** (`sipminp.p4`), so the shape is sound and pinning materially helps the
allocator.

`sc/sc_k1.p4` is that chain written into `sc_x0`, entirely in ingress. It fails. Emptying one
feature at a time says why:

| file | change from `sc_k1` | result |
|---|---|---|
| `sc/sc_k2.p4` | bloom filter emptied | compiles, 8 s |
| `sc/sc_k10.p4` | **only the bloom's hash computations emptied** | compiles, 9 s |

The bloom's CRC hashes and the chain's `@in_hash` rotates compete for hash units. When a rotate
cannot get one the compiler slices the operand instead, and the slicing propagates to everything
computed from it. That is the mechanism behind every fragmentation we chased. Our ingress asked
for 26 `@in_hash` sites; the expert's asks for about ten, because it runs two rounds in ingress
and two in egress.

`sipfull.p4` closes it: the chain plus the time-delta register plus the bloom, with the rounds
split across the two pipelines, compiles in 3 s.

## Does the unrolled chain fit if it uses egress? (`synthesized/smartcookie-unrolled.p4`)

Yes. The twelve rounds written out linearly, 2 in ingress and 4 in egress per lap, two laps and
**one** recirculation, compiles in 7 s (19 ingress stages, 18 egress). The rolled ground truth
needs two recirculations, so the unrolled form is not the thing that made SmartCookie infeasible.

Getting there took four compiles, and every failure was "supports up to 20 stages, using 21" or
"using 25" -- never PHV, never hash units. Two rules came out of it:

- A SipRound is four dependency levels, so twelve rounds are 48 and cannot fit one pass over both
  pipelines (40 stages) however they are allocated.
- Mutually exclusive branches share stages, but only from where they start. Emitting the
  recirculated lap *before* the first pass's clock/bloom/triage block took ingress from 21 stages
  to 18; the same reordering in egress took it from 21 to 18. The order alternative code paths are
  emitted in decides whether a program fits.

It dispatches through synapse's existing pass identifier, `hdr.recirc.code_path`, in both ingress
and egress, with no counter field of its own: the mechanism the synthesizer already emits for
multi-pass solutions is enough for an unrolled loop.

It passes `tests/smartcookie.py`. Getting there turned up the worst silent miscompile of the lot:
**bf-p4c split a `Register` across two stages**, allocating `Ingress.bf_row_0` SRAM in stages 4 and
5 with a stateful ALU in each, so the bloom's write and its read landed on different copies of the
state and never saw each other. 0 errors reported. Putting each row's read and write in a single
table, as actions chosen by a key, puts the register back in one stage and fixes it. Doing that also
showed that **an action using the hash distribution unit cannot be a table's `default_action`**.

## A crash the nested-pass fix exposed

Making the recirculation passes reachable (commit `e76cbb4fc`) turned six of the fourteen
multi-pass solutions from failing to compiling, and turned one, `nat-f40000-c100000-zipf0_6`,
into a bf-p4c internal compiler error with no message.

Bisected: pass 2 is the trigger. Inside it, either dropping the fourth register write or dropping
the byte-granular header writes makes it compile, and one, two or three register writes are fine
where four crash. Two independent fixes work:

- pack the vector's 32+32+16+16 registers into 32+32+32, three stateful ALUs instead of four;
- write the header fields whole instead of byte by byte.

The second is the one we took, because it needs no change to the register layout and no answer to
the byte-order question. Folding just the all-constant byte writes into one assignment
(`hdr.hdr1.data3 = 32w0x01020304` in place of four byte writes) is enough to clear the crash.
bf-p4c crashing rather than reporting an error is its own bug.

## What we ruled out

Recorded as prose because the artifacts were not worth keeping. None of these fixed the problem:

- **Reusing metadata slots more tightly.** Program-wide liveness, per-pass slot pools, and a
  linear-scan reallocation down to the true floor of 13 slots: no effect. The allocator was
  already near optimal, and around 30 of 48 32-bit containers is comfortably inside budget.
- **Giving every value its own field.** The opposite of the above: 160 metadata and 34 state
  fields. Compile time went to 18 minutes and it still failed.
- **Container pragmas.** `@pa_container_size` and `@pa_atomic` on slots and state fields do not
  fix the slicing, although pinning genuinely helps when the fields are few and dedicated.
- **Keeping recirculated state in a separate header**, in its own parser state, or pinned.
- **One statement per action**, and merging every run into one action per dependency level. The
  stage arithmetic improves, the slicing does not.
- **Splitting the traversals** so each fits in 20 stages. Six traversals, all within budget, same
  failure.
- **Chasing sub-word accesses one at a time**: whole-field port and address swaps, splitting a
  header field into aligned pieces, snapshotting a field before a byte read. Each moved the error
  to the next value, and a copy re-links the two fields so the fragmentation follows.

The common thread is that all of them tried to make the *unrolled* chain fit. It does not,
because a shared container must satisfy the union of the constraints of every value that ever
lives in it, and the unrolled chain has around 160 values against the loop's nine.

## Other target rules found along the way

- The hash unit's immediate pathway carries **32 bits per table**, summed over the table's actions:
  two 32-bit `@in_hash` actions in one table give "the number of bits required to go through the
  immediate pathway 64 ... is greater than the available bits 32", after which bf-p4c crashes with
  SIGSEGV instead of exiting. This is why the ground truth splits its final xor across two tables.
- A hash operation **can** sit in a keyless table: the ground truth's round actions each hold an
  `@in_hash`, are called bare from the apply block, and compile to keyless `hash_action` tables.
  (An earlier note here claimed the opposite.) What it cannot be is a table's `default_action`:
  "Cannot specify bf_query_0 as the default action, as it requires the hash distribution unit".
- A `Register`'s read and write must live in **one table**. Left in separate keyless tables they can
  be placed in different stages, and bf-p4c then duplicates the register rather than refusing:
  `Ingress.bf_row_0` got SRAM in two stages with a stateful ALU in each, silently splitting the
  state in two. 0 errors reported.
- `@in_hash` is needed for a multi-operand expression that would otherwise span stages
  (`a ^ b ^ c ^ d` is one hash op but three ALU instructions, and an action cannot span stages).
  It is *not* needed for a slice-and-widen read of an intrinsic, nor for a byte read feeding a
  table parameter; both were tried without it and compile and pass the model test.
- `@in_hash` accepts a non-byte-aligned rotate but rejects an aligned one
  ("source of modify_field invalid"); aligned rotates are written bare.
- Subtraction cannot take a table parameter as action data; addition can, so `a - k` becomes
  `a + (-k)` with the controller storing the two's complement.
- A condition inside an action must be a simple comparison on action data.
- A 32-bit inequality does not fit a gateway's 4 bytes + 12 bits, so accepted ranges become
  constant table entries.
- A `Checksum()` in the deparser can neither read nor write a slice, and the two fail differently:
  a sliced **input** is a hard error ("unexpected type of parameter ... in Checksum"), while a
  sliced **output** compiles with 0 errors and is silently ignored, leaving the original checksum
  on the wire. Fields a checksum touches must be their own header fields.
- Statements inside an action execute in order, so `a = b; b = a;` duplicates `b` instead of
  swapping; but a single write of a field in terms of itself
  (`ports = ports[15:0] ++ ports[31:16]`) is one operation and is correct.

## Hash-distribution units across mutually exclusive branches (`hdu1.p4` .. `hdu4.p4`)

Question: SmartCookie's BDD carries the SipHash chain twice, once on the SYN path and once on the
cookie-check path, and the two copies are the same ops on the same packet bytes. Each 32-bit
`@in_hash` rotate takes 2 of a stage's 6 hash-distribution units, so one chain (36 hash rotates)
costs 72 units and two cost 144 of the 120 the pipeline has. Does bf-p4c let two copies in
mutually exclusive branches share units? Three independent chains of 12 dependent rotates
(3 rotates per stage, 6 of 6 units) stand in for the SipHash chain.

| toy | shape | result |
|---|---|---|
| `hdu1` | one copy, unconditional | compiles, 12 stages |
| `hdu2` | two copies in `if`/`else`, same ops on the same fields | compiles, 12 stages: both copies' tables sit in one stage and use the **same six units** (`hash_dist(0,1)`, `(2,3)`, `(4,5)` in both) |
| `hdu3` | two copies, different rotate amounts | fails: "supports up to 20 stages, using 25" |
| `hdu4` | two copies, same ops, each on its own fields (`a/b/c` vs `d/e/f`) | fails: "using 26" |

Takeaways: a hash-distribution unit is shared between mutually exclusive tables only when the
hash expression is the same (same function, same input field). The same computation on different
fields is a different hash and gets its own units, and when a stage has none left the second copy
is pushed past the first, into stages the chip does not have. So two copies of the chain fit only
if they are emitted as one computation on one set of fields; synapse's per-path symbols
(`rotated__16` vs `rotated__107`) make them `hdu4`.

### Does the action matter, or the field it reads? (`hdu5.p4` .. `hdu8.p4`)

Question: is a rotate shared because the two branches call the same *action*, or because they
hash the same *field*? Same chains as above.

| toy | shape | result |
|---|---|---|
| `hdu5` | the same no-argument actions called from both branches | compiles, 13 stages, units shared |
| `hdu6` | one parameterized action `r5(in x, out y)`, each branch passing its own fields (`a/b/c` vs `d/e/f`) | fails, 26 stages, no sharing |
| `hdu7` | the same parameterized action, both branches passing the same fields | compiles, 13 stages, units shared |
| `hdu8` | each branch writes its own inputs into shared fields (`init_p` / `init_q`), then both call the one chain | compiles, 13 stages, units shared |

Takeaways: the action is irrelevant; a direct call with field arguments is inlined and the hash
unit is configured for the field it reads, so `hdu6` is `hdu4` with extra steps. What is shared
is a hash *of a given field*. Two computations that differ in an input can still share the chain
by selecting the input into one field first (`hdu8`), which costs no stage here and is what the
ground truth's `msg3_sel` does.

## An add of a wide constant, bare and in an action (`addc1.p4` .. `addc3.p4`)

Question: SmartCookie's cookie check computes `ack - 1`, which the BDD carries as an add of
`0xffffffff`. Does that shape compile, in the ingress and in the egress?

| toy | shape | result |
|---|---|---|
| `addc1` | ingress action: `meta = 32w0xffffffff + hdr.tcp.seq` | compiles |
| `addc2` | egress action: the same | compiles |
| `addc3` | egress action: `hdr.tcp.seq - 32w1` | compiles |

Takeaway: the shape is fine inside a named action. The synthesized SmartCookie failed on it only
where the emitter wrote it as a bare assignment in the apply block: bf-p4c synthesizes an action
for a bare statement and then rejects the wide constant there as "multiple action data
parameters". The emitter now puts every materialized header value in an action of its own.

## Where a value next to a hash rotate may live (`cutA.p4` .. `cutF.p4`)

Question: the synthesized SmartCookie's actions are rejected for "too many sources" wherever an
ALU op mixes a metadata value with a value a hash-unit rotate has cut into slices. Which home
makes a value safe next to a rotate? All are `phvW4` with the homes changed.

| toy | shape | result |
|---|---|---|
| `cutA` | the two rotates' inputs as fields of a header that is never valid nor emitted | fails, `x` needs too many sources |
| `cutB` | `@pa_container_size(32)` on the two rotates' inputs, left in metadata | fails the same way |
| `cutC` | `cutA` with the header set valid at init | fails the same way |
| `cutD` | every value of the chain, hash outputs included, in a header field (never valid, never emitted) | compiles |
| `cutE` | every ALU result in a header field, only the hash outputs in metadata | compiles |
| `cutF` | `cutD` plus an egress that writes and reads the same never-valid header | compiles |

Takeaways: what an ALU op reads next to a sliced value must itself be one container, and only a
header field is guaranteed one: the allocator packs a header's fields whole "because of the
structure of the header", whether or not the header is ever valid, emitted or parsed. A metadata
field, `no_split` by pragma or not, is still counted one source per slice. The hash unit's own
inputs are read by the hash crossbar, not the ALU, so they may stay in metadata (`cutE`).

## Rounds of HalfSipHash in a state header (`stA.p4`, `stB.p4`, `stA4.p4` .. `stD4.p4`, `L1.p4` .. `L4.p4`)

Question: does a chain of rounds compile when every value lives in a slot of one state header,
in each of the layouts the emitter could produce?

| toy | shape | result |
|---|---|---|
| `stA` | three rounds in slots of a header that is never valid nor emitted | compiles |
| `stB` | the same, the header set valid at the start, emitted, extracted by the egress parser, invalidated before the packet leaves | compiles |
| `stB4` | four rounds (the ground truth's per-gress maximum), every value in a header slot, byte-aligned rotates in `@in_hash` | compiles |
| `stA4` | `stB4` with the header never valid, never emitted, never extracted | compiles |
| `stC4` | four rounds in the ground truth's exact discipline: adds and hash rotates write metadata temporaries, xors and bare byte-aligned rotates write the header state | compiles |
| `stD4` | four rounds, every value in a header slot, byte-aligned rotates bare | compiles |
| `L1` | `stC4` with the add results in header slots: a metadata hash output and a header value meet in each xor | compiles |
| `L2` | `stC4` with the rounds called from both branches of an `if` | compiles |
| `L3` | `stC4` with the slots' roles rotating every round (a slot holds a different state word each round) | compiles |
| `L4` | `stC4` split: two rounds in the ingress, the state header carried to the egress, two rounds there | compiles |

Takeaways: every layout compiles, including the one the synthesized program was rejected on
(`L1`: a metadata hash output xored with a header slot). The toys never reproduce the synthesized
program's failures because they carry a dozen sliced fields where the synthesized program carried
32 to 39 per gress (`sc-walk/README.md`): with that many, bf-p4c rejects the action even after the
metadata field is pinned to one container, and fails PHV allocation on the cluster once every
field is a header field. The fix is on synapse's side: fewer distinct fields tied to the chain.

## How many sliced fields fit (`stN9.p4` .. `stN32.p4`, `stX8.p4`)

Question: the synthesized SmartCookie, with every computed value in a header slot, fails PHV
allocation on one supercluster of 22 (egress) and 16 (ingress) 32-bit fields sliced twelve ways,
while the ground truth's cluster of 11 allocates. Is it the number of fields alone?

| toy | shape | result |
|---|---|---|
| `stN9`, `stN10` | `stD4` with every write to a fresh slot, round-robin over 9 / 10 header slots | compile |
| `stN11`, `stN12`, `stN16`, `stN24`, `stN32` | the same over 11 or more slots | fail PHV allocation |
| `stX8` | `stD4` (8 slots) plus a header of eight 32-bit fields xored once into the state | fails PHV allocation |

Takeaways: the count is all that matters. The cluster of `stD4` holds the slots plus five other
fields (`ipv4.src`, `ipv4.dst`, `tcp.seq`, `o.f0`, `o.f1`); 15 fields allocate, 16 do not, and
`stX8` fails at 21 with no slot ever changing its role. A supercluster of 32-bit fields sliced by
the hash rotates has to fit one PHV group of sixteen 32-bit containers, so the whole chain of a
gress -- its state, its temporaries and every packet field or register value it reads -- may tie
together at most 15 fields. The ground truth's 11 is the budget spent well: four state words,
four temporaries, three inputs. Synapse's slot allocation and the fields it lets into the chain
have to stay under that line.

### Written fields against read-only inputs (`stW10.p4`, `stX2.p4`, `stX5.p4`, `stP12.p4` .. `stP14.p4`)

Question: is the ceiling a count of fields, or of containers of a kind? bf-p4c's status at the
failure shows Tofino 2's normal PHV as 48 W, 48 B and 72 H containers, four groups of 12, 12 and
18, next to 16 mocha and 16 dark of each width, four per group.

| toy | shape | result |
|---|---|---|
| `stP12` | 12 slots, the output header written from constants: 12 written fields, 3 read-only inputs | compiles |
| `stP13`, `stP14` | 13 / 14 slots, the same | fail |
| `stW10` | `stN10` with its three inputs also written by an ALU op: 15 written fields | fails |
| `stX2` | `stD4` plus 2 read-only fields: 10 written, 5 read-only | compiles |
| `stX5` | `stD4` plus 5 read-only fields: 10 written, 8 read-only | fails |

Takeaways: a sliced supercluster lives in one PHV group. A field an ALU writes needs one of the
group's 12 normal 32-bit containers; a field only the parser writes can take one of its 4 mocha
containers, and spills into the normal ones past that. So per gress the chain may tie together
at most 12 ALU-written 32-bit fields -- state slots, temporaries and every packet field the NF
both feeds into the hash and rewrites -- plus four read-only inputs. The ground truth spends 8
written (`v0..v3`, `a0..a3`) plus `data2` and `msg`, and reads the rest.

## Two laps on one state header (`scx-*.p4`, `stM.p4`, `stH.p4`, `stQ.p4`, `stR12.p4`, `stR12u.p4`, `stK12.p4`, `stCS12.p4`)

Question: the synthesized SmartCookie's ingress cluster has 11 slots and two read-only inputs,
under the budget above, and still fails. What about it do the toys lack? The `scx-*` files lift
the synthesized program's ingress compute actions verbatim into a bare skeleton, with each code
path's call sequence as a branch of an `if` on the port.

| toy | shape | result |
|---|---|---|
| `scx-lap2` | one second-lap sequence (23 actions) alone | compiles; 9 slots in normal containers, the two slots it only reads and `data1` in mocha |
| `scx-2laps2` | both second-lap sequences, one per branch | compiles |
| `scx-all`, `scx-lap12`, `scx-lap1a`, `scx-lap1b` | the first lap's sequence, whole or halved, next to a second-lap one | fail PHV allocation |
| `scx-one-113` .. `scx-one-118` | one first-lap action next to the second-lap sequence | 115 and 118 fail, the others compile |
| `scx-118-rot8`, `-hash`, `-move`, `-fromconst`, `-from8`, `-fromhdr`, `-late` | that action's statement with the op, source or stage changed, the destination `s32_5` kept | all fail |
| `scx-118-fresh`, `-to1`, `-to8`, `-to10` | the same statement writing another slot | all compile |
| `stM`, `stH`, `stQ` | two branches on the same slots and stages: sources rotated, hash and ALU writes swapped, add and xor swapped | compile |
| `stR12`, `stR12u` | `stP12` with the state header also extracted on a recirculation port, 4-byte aligned or 2 bytes off | compile |
| `stK12`, `stCS12` | `stC12` with state words set from constants alone, and with the wide constants applied to slots | compile |

Takeaways: what breaks the skeleton is any write, of anything, in any stage, to a slot that the
other branch reads before it first writes it (`s32_5`: read in stage 1, written from stage 2 in
the second lap). Writing a slot the other branch writes first, never writes, or a fresh one is
fine. The allocator uses a header field's container for something else until the field's first
write, and the second-lap sequence alone only allocates because of that room; the extra write
takes it away. The real program has no such room at all -- `parse_recirc` and the egress parser
extract the state header, so every slot is live from the parser -- and its other 32-bit packet
fields (`hdr0..hdr2` are cut into 32-bit chunks) take the mocha containers first. Its budget is
therefore the twelve normal 32-bit containers of one group, for slots and inputs alike. None of
the other suspected shapes (constants, two paths, recirculation parsing, alignment) matters.

Consequence, now in synapse: the ALU cluster may hold nothing but the state slots. Every value
that enters a hash chain from outside -- a packet field, a register's value, the clock -- is read
by the hash unit, as the ground truth's `time_read` reads the clock: an xor that takes it is
computed in `@in_hash`, any other op has it loaded into a slot by `@in_hash` first
(`TofinoModuleFactory::is_hash_chain_node`).

## One rotation per pair of words (`scy-*.p4`, `stHX.p4`, `stSame*.p4`, `stR10.p4`, `stP10.p4`, `stP11.p4`, `stC12.p4`, `st6P10.p4`)

Question: with every outside value entering through the hash unit, the synthesized program's
cluster is 11 slots and nothing else per gress, under the budget above, and PHV allocation still
fails on all of it (264 slices). The `scy-*` files lift regen 13's ingress compute actions verbatim
into the bare skeleton: `scy-all` with every code path, `scy-b0` .. `scy-b3` with one path's call
sequence each, `scy-pN` with the first N actions of `scy-b1`, `scy-wN` with actions N..15 of it.
The `-x` variants also extract the state header in the parser on a recirculation port, so no
slot has room before its first write, as in the real program.

| toy | shape | result |
|---|---|---|
| `scy-p6`, `scy-p10`, `scy-p14` | prefixes of the second-lap sequence | compile |
| `scy-p15` .. `scy-p22`, `scy-b1` | longer prefixes, the whole sequence | fail PHV allocation, with or without `@pa_container_size` / `@pa_no_overlay` on the slots (`-csize`, `-noovl`, `-both`) and with the header parsed (`-x`) |
| `scy-w5`, `scy-w12` / `scy-w9` | actions 5..15 and 12..15 / actions 9..15 | compile / fails: not monotonic in the statements |
| `scy-15a_rot8`, `scy-15b_hash` | action 15 cut to its byte rotate, to its hash rotate | compile |
| `scy-15c_add`, `scy-15d_alu`, `scy-15e_xor` | action 15 cut to the add `s32_6 = s32_0 + s32_2`, the add and the rotate, the add turned into an xor | fail |
| `scy-15f_no0`, `scy-15h_no2`, `scy-15l_rot16` | the add with either operand replaced, or a byte rotate into `s32_6` instead | fail |
| `scy-15g_fresh` | the same add writing a fresh slot | compiles |
| `scy-15i_norot`, `scy-15j_noxor` | action 14's byte rotate, or its xor, reading another slot than `s32_6` | fail |
| `scy-15k_prev10` | the previous value of `s32_6` (action 13's add, read by action 14) moved to a fresh slot | compiles |
| `scy-gt` .. `scy-gt6` | `scy-p15` re-homed in the ground truth's discipline (adds and hash rotates into pinned metadata temporaries, xors into header words, copies where a value is needed as both) | `scy-gt6` compiles, also without the header pragmas (`-nohp`) and with the header parsed (`-x`, all 12 normal 32-bit containers of a group used) |
| `scy-b1-gt` | the whole sequence re-homed the same way: 12 written words and 5 inputs | fails: past the budget |
| `gt-ahdr` | the ground truth with `a0..a3` moved from metadata into `recirc_state` | compiles: metadata is not what its discipline needs |
| `scy-p15-alt` .. `scy-p20-alt` / `scy-p21-alt`, `scy-p22-alt`, `scy-b1-alt*` | prefixes re-homed under strict alternation (every ALU statement reads one kind of word and writes the other; `-altp` pinned, `-altva` hash rotates v to a, `-altm` metadata a) | compile / fail |
| `scy-p18-alt12`, `scy-p18-alt12r` | `scy-p18-alt` with a twelfth written word, or a packet word, joining the cluster | compile: the count is not what fails the longer ones |
| `scy-p15-one`, `scy-p22-one`, `scy-b1-one` | one pool of 9 words by live range, a word never written from another word at two rotations | compile |
| `scy-b1-onefree` | the same pool without that rule: 7 pairs at two rotations | fails |
| `stHX`, `stSame`, `stSame2` | `stD4` with a message word xored in by the hash unit; with one action reading a state word both rotated and aligned | compile |
| `stR10`, `stP10`, `stP11`, `stC12` | `stP12` parsed on a recirculation port, with 10 or 11 slots, with SipHash's wide initial constants | compile |
| `st6P10` | `stP10` with six rounds | fails on stages, not PHV |
| `devdep` | a table writing a device id, and work guarded by a test on that id | the guarded work is placed in the table's own stage (the gateway lands a stage later), so a test on a table's result does not, on its own, delay what it guards |

Takeaways: every write, of anything, into `s32_6` after action 13 fails, a write into a fresh slot
does not, and moving action 13's value out of `s32_6` makes the write fine again -- the same
shape as the `scx-118` finding. bf-p4c's own debug output (`-Xp4c=-Tallocate_phv:5
-Xp4c=-Taction_phv_constraints:5`) names it: "Packing failed because `s32_6[16:16]` and
`s32_6[0:0]` would (conservatively) need to be aligned at the same position in the same
container", error code `OVERLAPPING_SLICES` from
`ActionPhvConstraints::check_and_generate_conditional_constraints`
(p4c, `backends/tofino/bf-p4c/phv/action_phv_constraints.cpp`). The allocator places the words
of a sliced cluster one at a time. When a destination is placed before one of its sources, it
records where that source's slices will have to sit relative to the destination's container. A
source that one op takes aligned (an xor, an add, a move) and another op takes byte-rotated (a
`++` of two slices) into the same destination needs two positions at once; the check is
conservative, the destination has no container that works, and the whole cluster fails. Whether
it bites depends on the placement order: `scy-p14` carries eight such pairs and allocates, the
ground truth four (`v0 = a0 ^ m` next to `v0 = a0[15:0] ++ a0[31:16]`), and one more statement
anywhere reorders the placement (`scy-w9` against `scy-w5`). The ground truth's two kinds of
words are not the point either: the same discipline fails on the whole sequence, and the ground
truth still compiles with its temporaries in the header. What its discipline buys is few such
pairs; a single pool by live range that refuses a slot where a source would be taken at a second
rotation (`rehome3.py`, `scy-*-one`) compiles the whole sequence in 9 words.

Consequence, now in synapse: `plan_value_homes` records, per gress, the rotation at which each
state word is written from each other word, in both directions (a value planned after one of its
readers, as a path that reuses another's op is), and never gives a value a slot that would take
one of its sources, or be taken by one of its readers, at a second rotation. The rotation of an
aligned op is 0, of a rotate or a left shift its amount, of a right shift or a cast of a field's
top bits the width minus the amount; hash-unit ops read at no alignment and are exempt. The
emitter, for its part, no longer sends a reader to another variable that happens to hold the same
value once the planner gave the value a slot (the "held elsewhere" shortcuts), and in walk mode
checks every statement's words against the plan. The pairs cost slots -- 14 in the egress where 10
values were ever live at once, greedily -- so the slots are chosen by a depth-first search over
each path's values that backtracks when a gress would touch more than 12 words (a few hundred
nodes on this plan), with a shift rotate's `or` written over its `shl` half and a value written
over an operand that dies at it; the regenerated program then touches 12 words in the ingress and
10 in the egress; with the pairs right but 14 egress words (regen 19 of the walk) bf-p4c's
failure had already narrowed from both gresses to the egress alone.

## What a gress's chain may hold (`scw-11.p4`, `scw-i12.p4`, `scw-e12.p4`, `scy-b1-onefree-hash.p4`, `scy-b1-onefree-ctrl.p4`)

Question: a synthesized SmartCookie with 12 state words per gress, the budget above, fails PHV
allocation, and the same program with 11 compiles. Which count is wrong? And is the
one-rotation-per-pair rule about every write, or only the ALU's? Both were first read in the
compiler's source -- the Tofino backend is open (p4lang/p4c, `backends/tofino/bf-p4c/phv`); it is
not the 9.13.4 binary, so what it says is a hypothesis until a toy agrees -- and then tested here.

| toy | shape | result |
|---|---|---|
| `scw-11` | the program with 11 state words in both gresses | compiles |
| `scw-i12` | 12 in the ingress, 11 in the egress | compiles |
| `scw-e12` | 11 in the ingress, 12 in the egress | fails PHV allocation |
| `scy-b1-onefree-hash` | `scy-b1-onefree` (seven pairs at two rotations, fails) with the byte rotates of those pairs through the hash unit | compiles |
| `scy-b1-onefree-ctrl` | the same actions and calls, those rotates back on the ALU | fails |

Takeaways: a supercluster goes into one container group (`SuperCluster`, `phv/utils/utils.h`;
`tryAllocSlicing`, `phv/allocate_phv.cpp`), and a Tofino 2 group of 32-bit containers is twelve
normal, four mocha and four dark (`specs/phv_spec.cpp`). bf-p4c's cluster dump
(`-Xp4c=-Tmake_clusters:4`) shows what the chain's cluster holds besides the state words: a packet
word an ALU statement reads next to a state word, with every 32-bit field of its header (the
header is one slice list: `hdr1.data5` and `data6` bring `data0` and `data1`), unless the reads
shift or slice it, which makes it solitary and leaves it alone (`hdr2.data0`, read as `>> 16` and
`[15:0]`). A state word is written by the ALU or the hash unit and needs a normal container; a
packet word is only parsed and takes a mocha one, then a normal one once the four are gone. So a
gress fits when `state words + max(0, packet words - 4) <= 12`: the ingress of `scw-i12` holds
12 + 4, the egress of `scw-e12` 12 + 5. The rule agrees with every synthesized SmartCookie whose
outcome is known (five that compile, three that do not). What the hash unit reads ties nothing:
its operands are no PHV sources (`ConstraintTracker::add_action`, `phv/action_phv_constraints.cpp`),
which is also why the pair rule is the ALU's alone -- a destination written from a source at two
rotations is refused only when both writes are ALU statements, the source is still unplaced and
the destination's bytes share a container (`check_and_generate_conditional_constraints`). A byte
rotate through the hash unit costs two hash-distribution units and the action's one `@in_hash`.

## Separate ifs on one field versus an if / else-if chain (`mx_else.p4`, `mx_sep.p4`)

Does bf-p4c treat `if (x == 80) { A } if (x == 81) { B }` as mutually exclusive, the way it treats
`if (x == 80) { A } else if (x == 81) { B }`? Each branch runs a chain of twelve dependent ALU ops
writing the same thirteen fields. Yes: the else-if chain takes 13 ingress stages, the separate ifs
14 -- one stage for the second gateway, not a second chain. So a merged ladder arm may guard each
block's code with its own `if` on the code path. What costs stages there is where each branch
starts (see above): code emitted after another branch's later tables cannot use earlier stages.
