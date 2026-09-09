# What the ground truth does that the synthesized SmartCookie does not

**Question.** `smartcookie-unrolled.p4` compiles for Tofino 2 in 9.5 s. Synapse's output does not
compile at all. Rather than push the synthesized file forward one error at a time, read everything
bf-p4c says about the ground truth first, then ask what the synthesized program is missing.

**Method.** `fixes.py` applies named, individually-justified transformations to `base.p4`
(synapse's output, unmodified) and `run.sh` compiles the result. Every fix below is
semantics-preserving unless the entry says otherwise. Read `phv_allocation_0.log`, not just the
error summary: it names the constraint on each field.

## Are the `@pa_container_size` pragmas essential? No.

Stripping all 16 from the ground truth still compiles, 0 errors, and produces a **byte-identical
container assignment**. It only costs time: 9.5 s with them, 32.6 s without. They are a hint that
prunes the allocator's search, not a requirement, and copying them into synapse would buy nothing.

## The diagnostics are not violations

The ground truth's own log contains `diagnose_slicing found a unallocable super cluster` twice,
including the same 9-bit `hdr.recirc.ingress_port` split ours has, plus two bit-order rejections.
It then prints `allocation(default_alloc_config): Succeeded`. The allocator backtracks. What
distinguishes a failure is the *size* of the search:

| | ground truth | synapse (fixes 1-4) |
|---|---|---|
| unallocable superclusters | 2 | 30 |
| bit-order rejections | 2 | 306 |
| allocator configs tried | 1, succeeded | 3, all failed |

This is why three earlier hypotheses about action *shape* each changed nothing: they did not
shrink the search. Recorded so they are not retried: one `@in_hash` per action; moving ALU reads of
a hash-rotated value into their own action; `@pa_container_size` on the cut fields. All left the
count at exactly 84. The ground truth does what all three forbid -- it rotates `meta.a0` by 16 and
also reads it whole with an ALU -- and compiles.

Nor is slicing itself the problem. The ground truth's `hdr.recirc_state.v0` is sliced **twelve**
ways and all twelve pieces sit in one 32-bit container, `W6`.

## Results

| build | fixes | outcome |
|---|---|---|
| `base` | none (synapse's output) | 1 error |
| `g1234` | 1 2 3 4 | 84 slices unallocated |
| `g1234B` | + whole-field bloom key | 84 -- **falsified** |
| `g1234A` | + drop computed values from the cpu header | 60 |
| `g1234AD` | + write deparsed header fields through the hash unit | **PHV succeeds**; MAU errors |
| `g1234ADG` | + one `@in_hash` per action | **0 errors** |
| `g1234A2DG` | A2 instead of A (semantics-preserving) | **0 errors** |

Fixes, and the ground-truth observation behind each:
- **1** a whole-value read must be a whole field (the 32-bit ack straddled a guessed 24/24 boundary)
- **2** `x[31:24]++x[23:16]++x[15:8]++x[7:0]` is the identity; bf-p4c reads it as four sources
- **3** an 8-bit op stays 8-bit; the ground truth writes `data4[7:0] = data4[7:0] | 8w0x12`
- **4** a byte-aligned concat rotate is legal bare but still cuts its operand's container
- **A2** `hdr.cpu.X = meta.X` copies a cut metadata field into a `deparsed exact_containers`
  field, needing one PHV source per slice; route it through the hash unit. (**A** deletes those
  fields instead: it compiles but breaks the controller replay, so A2 is the fix and A is a probe.)
- **D** the ground truth writes its result into a deparsed header field as
  `@in_hash { hdr.hdr2.data2 = ctime ^ v0 ^ v1 ^ v2 ^ v3; }`, never as a plain ALU copy
- **G** the ground truth carries exactly one `@in_hash` (32 bits) per action; fix 4 over-applies and
  puts 64 bits through a table's immediate pathway. In synapse this is the existing
  `MAX_HASH_BITS_PER_ACTION` budget -- the real fix is to classify byte-aligned concat rotates as
  `Hash` ops rather than `ALU` ops, and the budget then enforces itself.

## The reason this result does not mean what it looks like

`g1234A2DG` compiles, but bf-p4c allocates **84 tables and reports "stages for egress table
allocation: 1"**, against the ground truth's 121 tables and 19 stages. The 22 compute actions in
`control Egress` are not in the binary. They were eliminated as dead code, and they are dead:

- `EgressParser` extracts `egress_state`, `hdr0`, `hdr1`, `hdr2` -- **not `hdr.recirc`**, although
  `recirc_h` sits in `synapse_egress_headers_t` ahead of `egress_state`;
- nothing in the egress calls `hdr.recirc.setValid()`, yet the egress ends by writing its eight
  results into `hdr.recirc.f32_0..7`;
- the egress sets no egress port and triggers no recirculation.

So the egress computes the second half of the SipHash chain and throws it away. That makes the
current 1-recirculation, 286.83 Mpps winner **incorrect**, explains why crossing to the egress
looked free to the search, and means the PHV pressure measured above is the ingress half only.

**Fix the egress/recirculation header protocol first.** Every number in this file should be
re-measured once the egress half is actually in the binary.

---

# Part 2: with the egress live

`fH` fixes the correctness bug above (`EgressParser` now extracts `hdr.recirc`, so the egress's
results are emitted instead of being eliminated). Every measurement in Part 1 was on a program
whose egress half was not in the binary; these are the honest numbers.

**A note on how success is judged.** `run.sh` now checks that `bf-rt.json` exists. bf-p4c can
print `0 errors, 7 warnings` on its own line and still refuse to emit a binary
(`Due to errors, no binary will be generated`), which made two builds look like passes. Re-verified
from artifacts: only `g1234ADG` and `g1234A2DG` ever compiled, and both have a dead egress.

| build | fixes | outcome |
|---|---|---|
| `g1234A2DGH` | Part 1's set + H | 66 slices unallocated, all in the **egress** |
| `g2_WDGH` | + W (33 deparsed writes through the hash unit) | PHV passes; **egress needs 23 stages of 20** |
| `gM1` | W restricted to the 8 genuinely-cut sources | 207 unallocated |
| `gK`/`gK2` | liveness slot pooling (191->15, or cut-aware 191->20) | fails: pooling concentrates cuts |

## The trade-off, stated precisely

Three constraints pull against each other, and every fix that relieves one tightens another.

1. **PHV cuts.** A rotate written as a concat cuts its operand. Copying a cut value into a
   `deparsed exact_containers` field (the cpu, egress_state and recirc headers) needs one PHV
   source per slice, and only two are available.
2. **Hash-distribution units.** `@in_hash` fixes (1), but a 32-bit `@in_hash` takes 2 of a stage's
   6 units, so only **3 per stage**. The table summary shows this directly: from stage 11 on,
   exactly 3 tables per stage. Wrapping 33 writes cost the 3 stages that overflowed the egress.
3. **Stage depth.** 20 per gress.

Synapse's own output uses 36 `@in_hash` (24 ingress / 12 egress), which is already in line with the
ground truth's 40 (20/20) -- so synapse's `@in_hash` *policy* is right, and fixes 4 and W are blunt
instruments that buy PHV with hash units.

## Why pooling did not work, and what the ground truth really has

Pooling 191 fields into 15 slots fails because a slot carries the union of the cuts of every value
that lives in it, and one sub-word boundary anywhere fragments the chain. Making it cut-aware
(a slot shared only between values with the same cut signature) still failed, because the signature
has to include shifts and plain copies, not just concat rotates: `meta.time` ended up cut eight
ways purely by being copied into a slot that a shift-form rotate also used.

The ground truth does not need any of this. It has **8 live 32-bit values per gress**, and SipHash
gives each word a fixed pair of rotations (`v1` only ever 27 and 19, `a0` only 16), so no value
ever accumulates a large cut set. It also never copies a cut operand into a deparsed field: its
recirculation state is always written as a *result* (`v0 = a0[15:0] ++ a0[31:16]`,
`v1 = a1 ^ a0`), never as a copy of a cut input. Ours writes `hdr.recirc.f32_0 =
rotate_left_144_x_out`, which is a rotate's cut *input*.

## Where this points

The ground truth runs **2 SipHash rounds in ingress and 4 in egress, twice** -- two laps. Ours
runs the whole 12 rounds in one lap, 58 ingress and 22 egress compute actions. The per-gress chain
is simply too long, and no local rewriting fixes that: `@in_hash` trades PHV for hash units, and
there are not enough of either at this chain length.

Note *why* synapse chose one lap: with the egress dead, crossing was free. Fixing `fH` in synapse
changes the cost of the crossing, and the search should be re-run before more hand-fixing. The
shift form of a rotate (`x<<n`, `x>>(32-n)`, `|`) is the other untried lever: it neither cuts its
operand nor uses a hash unit, at the cost of one extra stage -- synapse already emits it for some
rotates (`compute_rotate_left_140_shl`/`_or`).

## The shift form does not rescue it either

`fS` converts the rotates whose operand is copied whole into a deparsed header (7 of them) to the
shift form, with the dependent `|` in its own action (bf-p4c rejects `(x<<n)|(x>>(32-n))` in one
action: "action spanning multiple stages"). Result: 102 slices unallocated, worse than the 66
without it. So all three levers are exhausted -- `@in_hash` trades PHV for hash units, the shift
form trades them for stages, and pooling concentrates cuts.

## Conclusion: this execution plan does not fit, and no local rewrite makes it fit

With the egress live, `g2_WDGH` gets past PHV and then reports **ingress 20 stages, egress 23**,
against 20 available per gress. The ingress is exactly at the limit and the egress is over it, in
the same program. That is not a slicing problem; the plan puts too much of the chain in one lap.

- ours: 12 SipHash rounds in **one** lap -- 58 ingress + 22 egress compute actions
- ground truth: 2 rounds in ingress, 4 in egress, **twice** -- one recirculation

And synapse chose one lap for a reason that is now known to be wrong: with the egress dead
(the `fH` bug), crossing to it was free. Fixing `fH` inside synapse changes what a crossing costs,
so **the search must be re-run before any further hand-fixing** -- the plan, not the emitted P4, is
what is out of budget.

---

# Part 3: the cut-signature hypothesis, falsified — and the actual mechanism

**Hypothesis.** Synapse's unmodified output is already inside the hash budget (24 ingress / 12
egress `@in_hash`, floors of 8 and 4 stages, against the ground truth's 20/20 and 7/7). So the only
difference that matters is value count: 191 ingress / 80 egress fields against 8. Pool them into
slots, sharing a slot only between values with an identical cut signature, and it should compile.

**Result: it does not.** `fK3` pools 191->23 and 80->15 with clean signatures that match SipHash
exactly (`c27` = rotl 5, `c19` = rotl 13, `c24` = rotl 8, `c25` = rotl 7, `c16` = rotl 16). It
fails, and pinning the slots with `@pa_container_size` (`fP`, at 38 pins and again at the ground
truth's scale of 19) fails identically at 272 unallocated slices.

## Why, exactly

The failing statement names both halves of the mechanism:

```
Source 1: hdr.egress_state.op_xor_387_out[0:0] ... [31:25]   <- 8 slices, ONE container
Source 2..9: meta.meta_c25_0[0:0] ... [31:25]                <- 8 slices, EIGHT containers
```

1. A deparsed header field is `exact_containers`, so it stays whole and carries the byte grid
   {8,16,24}. Any compute value that passes through the crossing state, the recirculation state or
   the cpu header inherits that grid.
2. A non-byte-aligned rotate composes with it: {8,16,24} together with the same grid rotated by 7
   ({1,9,17}) and the rotate's own boundary 25 gives seven boundaries, so eight pieces.
3. Eight pieces is eight PHV sources. The limit is two.
4. The only escape is for the value to occupy **one** container, which needs either
   `exact_containers` (a header field) or `@pa_container_size` (a pin).

So cut *signatures* are the wrong model. Cuts unify across the whole chain because `a = b ^ c`
forces b and c into compatible layouts, so no grouping can keep them apart -- and indeed the
ground truth's own `recirc_state.v0` is sliced twelve ways. Slicing is harmless; being *split
across containers* is fatal.

## What the ground truth actually relies on

Three things together, none of which is the pragma by itself:

- its rotate operands are **header** fields (`hdr.recirc_state.v0..v3`), which are
  `exact_containers` and therefore whole by construction;
- its rotate results are **metadata** (`meta.a0..a3`), only four per gress, every one pinned;
- so every operand of every rotate is guaranteed to be a single container, and the byte grid never
  gets a chance to multiply.

That is why the pragmas look unnecessary when tested in isolation: with 8 values the allocation is
easy enough to find unaided, so removing them changes nothing. They are load-bearing at scale, and
scale is what we have.

## Where this leaves it

Pins are scarce -- 64 32-bit containers per pipe, and the ground truth already uses 36 of them.
Ours needs 23 + 15 slots pinned *plus* the header fields, and 19 pins already starve
`hdr.hdr2.data2` and `meta.time`. Pooling cannot go below 23 in the ingress because that is the
measured peak liveness at action granularity, and packing several ops into one action is what
raises it.

Peak liveness is set by how much of the chain runs in one pass. The ground truth runs 2 rounds in
ingress and 4 in egress per lap and needs 8 values; we run all 12 in one lap and need 23. **The
value count is a consequence of the plan, not of the emitter** -- which is the same conclusion Part
2 reached from stage counts, now reached independently from container counts.

---

# Part 4: a compiling solution

    ./run.sh gX1 1 2 3 4 H A2 D G W      ->  COMPILED

Synapse's output plus nine hand fixes, **none of which deletes any computation**, compiles for
Tofino 2 with the egress genuinely live.

| | ground truth | gX1 |
|---|---|---|
| stages (ingress / egress) | 18 / 19 | **17 / 18** |
| tables | 121 | 108 |
| 32-bit containers (ingress / egress) | 19 / 17 | 44 / 20 |
| egress field slices in PHV | -- | 241 (a dead egress shows 26) |

## What finally did it, and why the earlier attempts were backwards

Every attempt to *reduce* the number of chain values made things worse -- pooling 191 -> 23 gave
272 unallocated, moving them into headers 304, moving all of them 360. The reason is that a pooled
slot is touched by many operations and so accumulates every boundary any of them imposes, while an
unpooled value is touched by one or two and keeps a small set. Cuts unify across the chain anyway
(`a = b ^ c` forces b and c into compatible layouts), so grouping by cut signature cannot separate
them. Pinning does not rescue it either: `@pa_container_size` marks a field `solitary`, so 19 pins
consume 19 32-bit containers exclusively and starve `hdr.hdr2.data2` and `meta.time`.

What was actually in excess was the **carried state**, not the metadata:

| 32-bit fields in carried headers | ground truth | synapse |
|---|---|---|
| `cpu_h` | 0 | 12 |
| `egress_state_h` / `recirc_state_h` | 5 | 14 |
| `recirc_h` | 1 | 9 |
| total | **6** | **35** |

Each of those is `deparsed exact_containers`, so each needs a whole container and cannot be split.
Thirty-five of them, plus the chain, is what exhausted the allocator. `fA2` and `fW` route every
write into such a field through the hash unit instead of copying a cut metadata value into it,
which is exactly what the ground truth does (`@in_hash { hdr.hdr2.data2 = ctime ^ v0 ^ ... }`).
That, with `fH` making the egress real and `fG` keeping one `@in_hash` per action, is the recipe.

## Status of each fix

Semantics-preserving by construction: **1** (same bits, re-cut field boundaries), **2** (an identity
concat), **3** (an 8-bit op stays 8-bit; only the low byte is ever read), **4**, **A2**, **D**, **W**
(all `@in_hash` wrappers -- same expression, different execution unit), **G** (splitting an action,
checked: no moved statement reads a field written in the same action). **H** is a bug fix.

**This is an argument, not a test.** `tests/smartcookie.py` on the Tofino 2 model is what would
prove the logic is intact, and it has not been run on this build.
