# Staging deparsed header operands into metadata

**Question.** The synthesized SmartCookie fails PHV allocation with

```
Action Ingress.compute_rotate_left_163 must be rewritten, because it requires too many sources.
  Source 1:    meta.meta_sn_11[7:0] ... [31:27]   <- 7 slices, ONE container
  Source 2..8: meta.meta_s16_1[7:0], [15:8], ...  <- 7 slices, SEVEN containers
However, Tofino2 supports only two PHV sources.
```

The offending supercluster holds **44 fields sharing one 7-boundary grid `{8,16,19,24,25,27}`**:
22 pooled chain slots, 17 deparsed header fields, and 5 stragglers. `{16,19,24,25,27}` are the
chain's own rotate cuts, inherent to SipHash. `{8,16,24}` is the deparser's byte grid, and it is
the header fields that bring it in.

**Why they bring it in.** A deparsed header field is `exact_containers`: it cannot be split, and it
carries the byte grid. `a = b ^ hdr.X` requires its operands to be *aligned* so the ALU can act
slice by slice, so the grid unifies with the chain's cuts and every value ends up needing seven
pieces. A *copy* is different: `a = hdr.X` needs one PHV source, not an aligned pair, so it does
not unify anything.

**Hypothesis.** Copy each header operand into metadata once, in the parser, and point the chain
arithmetic at the copy. It costs no MAU stage and no hash unit, and the copy should carry only the
chain's rotate cuts.

This is what the ground truth already does with its message word -- `action msg_src() { meta.msg =
hdr.hdr1.data5; }`, then `v3 = v3 ^ meta.msg`. It never mixes a packet field into the chain.

## `toy.p4` -- both halves confirmed

Compiles for Tofino2, exit 0. And in `phv_allocation_summary_0.log` the staged copy is

```
|MW0 |I | [18:0]  | ingress::m.sa[18:0]
|    |  | [31:19] | ingress::m.sa[31:19]
```

sliced **only at its own rotate cut (19)** and whole in **one container**. The header field it was
copied from is not in that cluster at all. So a parser-staged copy does escape the byte grid.

## `stage.py`

Applies it to a synthesized `sctest.p4`: declares the aliases, writes them in the parser state that
has just extracted the header, and rewrites header operands on the RHS of metadata assignments --
and only there, so writes, checksum lists, `pkt.emit` and the swap actions keep naming the header.

    stage.py <in.p4> <out.p4>
    -> ingress: 12 staged copies, egress: 8; 40 arithmetic sites rewritten

Correctness rests on ordering, checked in the emitted P4: the SipHash chain is applied at lines
1093-1111 and the packet rewrite (`swap_action_180/181`, the `hdr2.data1/data2` writes) at
1210-1219, so every ingress read wants the value **as it arrived** -- which is what the parser
copy holds. The egress reads `hdr2.data1` *after* the ingress rewrote it, and the egress parser
extracts the rewritten packet, so its copy is correct too.

`hdr.hdr2.data3[7:0] | 8w0x12` is left alone: an 8-bit op does not carry the 32-bit grid.

## `toy_hashwrite.p4` / `toy_shared_action.p4` -- the write side

Two more questions, because the read side alone did not settle the program.

*Does `@in_hash` on a write decouple the destination header from its sources?* Yes.
`@in_hash { hdr.w.b = m.r ^ m.sa; }` puts `hdr.w.b` whole in `W2` while `m.sa` sits in `MW0`
sliced only at its own cut. *Does it still decouple when the write shares an action with plain
chain ALU statements,* as it does inside a synthesized apply-block branch? Also yes -- same
result with three ALU statements beside it.

The contrast is in the same log: `hdr.w.a`, written by a **plain** copy `hdr.w.a = m.sd;`, comes
out sliced `[18:0]`/`[31:19]`, having inherited the chain's cut. So the rule is about the *kind*
of write, not about action packing: a plain write into a deparsed field couples it to the chain,
an `@in_hash` write does not. This confirms the premise behind `fW`/`fA2` in the parent study.

## Result on the real program: no change to the verdict

Staging removed 40 header-operand sites, and a unification closure over the emitted P4 then shows
**zero** deparsed header fields in either chain component. The ingress cut supercluster went from
44 fields / 17 headers to 41 fields / 2 headers.

It did **not** change the outcome: the same two actions fail with the same "too many sources", and
the ingress grid is still the same seven pieces. Staging is a correct and cheap transformation --
it costs no stage and no hash unit, and it is what the ground truth does -- but on its own it does
not make this program fit.

What remains is not a slicing problem. With the byte grid accounted for, the chain still carries
SipHash's own five rotate cuts, and 22 pooled slots all needing that same six-piece layout is more
than the allocator can co-locate. The ground truth needs 8. That difference is set by running 12
rounds in one lap instead of two, which is a property of the execution plan, not of the emitter --
the same conclusion the parent study reached from stage counts and from container counts.

See `tofino/experiments/gt-anatomy/README.md` for the rest. Two facts bound what else is
available:

- `@in_hash` is nearly exhausted -- ingress already has 48 hash-carrying actions, and at the
  measured 3 per stage that is a floor of 16 stages of 20. Room for about 12 more, against the 37
  sites that would need wrapping.
- `@pa_container_size` is not a lever. Re-tested on the non-degenerate program: the pragmas *are*
  honoured (every slot shows `solitary no_split`) and the failure is bit-identical with and
  without them.
