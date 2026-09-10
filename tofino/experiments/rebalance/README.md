# Cutting the pass where few values are live

**Question.** `tofino/exp-compute/GROUND-TRUTH.md` ends on an unbuilt piece of work:

> The plan should prefer pass boundaries where few values are live, and nothing in the score
> expresses that today -- "Memory Usage" in the score is data structure memory, not live state.
> That cost term is the next piece of work.

Before building that cost term into the search, apply it by hand to a synthesized SmartCookie and
see whether it does what the claim says.

**Method.** Synapse cuts the ingress->egress pass after the whole ingress hash block, 31 ops in.
`rebalance.py` measures the live set at every candidate boundary in that block and moves the cut to
the cheapest one.

    op in block :  0   1   2   3   4   5   6  ...  29  30  31
    live across :  0   3   3   4   6   8  10  ...  10  10  10   <- 31 is where synapse cuts

Cutting after op 2 leaves five values live, and three of them are packet fields the **egress parser
extracts for itself** while a fourth is the clock, which the egress reads from `global_tstamp`. So
the real payload is one value, plus one bare control-local (the delta a vector-table lookup
returns, which bf-p4c names for you as "declaration not found" if you forget it).

## Result

| | synapse | rebalanced | `smartcookie-unrolled.p4` |
|---|---|---|---|
| ingress actions | 100 | 71 | 79 |
| egress actions | 34 | 63 | 54 |
| ingress 32-bit metadata | 49 | 37 | 9 |
| egress 32-bit metadata | 28 | 50 | 10 |
| **`egress_state` fields** | **14** | **4** | -- |

The state header is the number that matters. Every field in it is `deparsed exact_containers`, so
each costs a 32-bit container for the life of the program, and the parent study measured that
pressure as the thing the allocator runs out of ("the recirculation header holds 21 32-bit fields,
and the chip has 80 32-bit containers for both gresses together").

## Five fields the crossing wrote that nothing read

Independent of the cut, synapse's own output carries `rotate_left_108_x_out`, `op_shl_380_a_out`,
`op_or_381_b_out`, `rotate_left_120_x_out` and `rotate_left_132_x_out` across the crossing, and
nothing anywhere reads four of them. That is a live-set over-approximation at the crossing and a
synapse bug in its own right: four deparsed 32-bit fields, permanently allocated, for nothing.

## What made the rewiring cheap

The egress body never reads `hdr.egress_state` directly -- it reads parser-staged `eg_md.stg_es*`
aliases (see `../parser-staging`). So moving the producers into the egress is a matter of pointing
those aliases at locally computed values instead of at the header, and the egress body itself does
not change.

## It does not compile, and the reason says what to do next

At cut 2 bf-p4c grinds in PHV allocation and hits a 400 s timeout -- the thrash signature, not the
40 s rejection the un-rebalanced build gives. The cut moved the problem rather than fixing it:

| 32-bit metadata | synapse | cut=2 | unrolled GT |
|---|---|---|---|
| ingress | 49 | 37 | 9 |
| egress | 28 | **50** | 10 |
| **total** | **77** | **87** | **19** |

**Redistribution cannot reduce the total, and the total is what is wrong.** No choice of cut point
in a three-pass plan gets 87 live values down to 19.

Merging the egress's two chain pools (see below) brings the total to 70 and the build still times
out, which is the same answer from the other direction: the count is not an allocation problem.

## Where our metadata actually goes

| | ours ingress | ours egress | GT ingress | GT egress |
|---|---|---|---|---|
| chain slots | 19 | 19 | 5 | 5 |
| staging aliases (`../parser-staging`) | 12 | 11 | 0 | 0 |
| other | 6 | 3 | 4 | 5 |

Four causes, in order of size:

1. **The ground truth keeps the hash state in a header.** `v0..v3` live in `recirc_state`, which it
   carries across the lap anyway, so the state costs nothing from the metadata budget: its chain
   metadata is `a0..a3` plus `msg`. Ours is all in `meta`/`eg_md`.
2. **Pooling reuses within a pass; the ground truth reuses across the program.** A pool is sized by
   peak simultaneous liveness in one pass, so six unrolled rounds in one ingress pass need 19 slots
   where two rounds writing back into the same four words need 5. This is the dominant cause and it
   is the pass structure, not the allocator.
3. **Staging aliases cost 23 fields**, a third of the ingress count -- the price of buying PHV
   alignment with PHV count.
4. **Two disjoint egress chain pools**, 15 native and 19 imported, which this script created and
   now merges: they are strictly sequential, handing over through the `stg_es*` aliases. Egress
   metadata 51 -> 33.

The ground truth reaches 19 by running **four** compute passes, not three: its egress control opens
with `if (hdr.recirc.code_path == SIP_PASS_2)`, so it runs on both laps.

| | passes | rounds per pass |
|---|---|---|
| `smartcookie-unrolled.p4` | ingress1, egress1, ingress2, **egress2** | 2 / 4 / 2 / 4 |
| ours | ingress1, egress1, ingress2 | ~6 / ~3 / ~3 |

Twelve rounds over four passes is three rounds live at a time. Over three passes, with the first
one fat, it is roughly double that no matter where the first cut goes.

So the next manual step is a **second crossing on the recirculated lap**: move the lap-2 ingress
chain into a `code_path`-dispatched egress block, which is this same script applied to the second
lap. Synapse's plan has one `SendToEgress`; the ground truth's shape needs two.

`rebalance.py <in.p4> <out.p4> [cut]`, default cut 2, applied on top of `handfix.py` and
`stage.py`.
