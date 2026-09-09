# Why the synthesized SmartCookie P4 does not fit in PHV

**Question.** Synapse now synthesizes a SmartCookie solution that splits its work across
ingress and egress (1 recirculation, 286.83 Mpps). It does not compile. Which of the remaining
differences from the hand-written ground truth actually block bf-p4c, and which are cosmetic?

**Method.** Take the synthesized P4 unmodified and apply one hand edit at a time, compiling each
step with bf-p4c 9.13.4 for Tofino 2 and reading the whole report, not the tail. Each step is
named below with the edit and the resulting error count. A step that does not improve the count
is recorded as falsified rather than dropped, because the falsifications are the useful part.

| step | edit | result |
|---|---|---|
| 0 | synthesized output, unmodified | 1 error: a composed value is an ALU operand |
| 1 | re-cut `hdr2` so the 32-bit TCP ack is one field, not `data2 ++ data3[23:16]` | that error gone; next one exposed, 174 slices unallocated |
| 2 | collapse the identity byte-concat `x[31:24]++x[23:16]++x[15:8]++x[7:0]` to `x` | 116 unallocated |
| 3 | do the 8-bit TCP-flags OR at 8 bits instead of widening it to 32 | 116, error moves on |
| 4 | wrap **every** concat rotate in `@in_hash`, not only the non-byte-aligned ones | **84** |
| 5 | split actions so each holds at most one `@in_hash` | 84 — no change, falsified |
| 6 | move ALU reads of a hash-rotated value into their own action | 84 — no change, falsified |
| 7 | `@pa_container_size(..., 32)` on all 27 cut fields | 84 — no change, falsified |
| P | **probe:** delete the two rotates that cut the cluster | **0 errors, compiles** |

## What the compiler says

At step 4 the 84 unallocated slices are exactly **28 fields x 3 slices**, one supercluster, every
member cut `[15:0] [24:16] [31:25]`. Only two statements in the whole program create those
boundaries -- a rotate by 7 and a rotate by 16 -- and the cut then propagates to all 28 through
the xor/add chain. Nothing else in the program fails to allocate.

## Takeaways

Steps 1-4 are real emitter defects and each one removed a specific, named compiler error. Step 4
corrects a rule recorded earlier: byte-aligned concat rotates are *legal* bare, but they still cut
the container, so `@in_hash` is about container integrity and not only about legality.

Steps 5-7 are falsified, and together they rule out the explanation I had been carrying. The
ground truth does everything they forbid: it rotates `meta.a0` by 16 and also reads it whole with
an ALU, in different actions, and it compiles. So the blocker is not the shape of any one action.

What is left is scale. The cut triples container demand for every value it reaches, and the
ingress is already near its 32-bit container budget in the version that compiles. The ground truth
carries **9** 32-bit ingress metadata fields and reuses them every round; the synthesized program
carries **197**, one per operation, all simultaneously live. Slot reuse by liveness is therefore
not an optimisation here, it is the remaining correctness-of-fit requirement.
