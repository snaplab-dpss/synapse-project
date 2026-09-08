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

`GROUND-TRUTH.md` describes `synthesized/smartcookie-manual.p4`, the hand-written SmartCookie that
compiles and passes `tests/smartcookie.py`, and lists what synapse cannot express about it. That
file is the point of everything below.

## Skeleton

`cmp12.p4`: ethernet + ipv4 parser, a few metadata fields, an `init` action and a forwarding
decision. Every toy is this file with the actions replaced.

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

## Does the unrolled chain fit if it uses egress? (`sc_unrolled.p4`)

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

Caveat, recorded rather than solved: the bloom filter stops working in this build although its
source is byte-identical to the working one, so `sc_unrolled.p4` passes every part of
`tests/smartcookie.py` except the recorded-flow path. See `GROUND-TRUTH.md`.

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
  (An earlier note here claimed the opposite.)
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
- A `Checksum()` in the deparser can neither read nor write a slice. The read side can be worked
  around by staging into metadata; the **write** side cannot, and is silently ignored, so the
  packet keeps its original checksum. Fields a checksum touches must be their own header fields.
- Statements inside an action execute in order, so `a = b; b = a;` duplicates `b` instead of
  swapping; but a single write of a field in terms of itself
  (`ports = ports[15:0] ++ ports[31:16]`) is one operation and is correct.
