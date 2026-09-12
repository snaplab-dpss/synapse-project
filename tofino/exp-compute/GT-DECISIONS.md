# The ground truth as a sequence of synapse decisions

*2026-09-12. Step 1 of `PLAN.md`. A document, not code.*

This is `synthesized/smartcookie-unrolled.p4` rewritten as the choices synapse's search would have
to make, over `bdds/smartcookie.bdd`, to arrive at it. It is the path the replay/interactive walk
follows, and every place it names a module that is not offered, or is offered and refused, is a
reachability blocker and a synapse change. Node ids are the BDD's; module names are the
`Tofino_*` entries of `Module.h`; a **decision** is a step where the search offers more than one
child and this document says which to take.

The walk is a debugging instrument for any heuristic, so nothing here assumes `max-tput`.

## The BDD, annotated

Node 2 is the root. Conditions are given by meaning; the exact expressions are in the dot.

```
2   ParserExtraction   ethernet (14 B)
3   ParserCondition    IPv4 and long enough                          else 229-230 drop
4   ParserExtraction   IPv4 (20 B)
5   ParserCondition    not UDP                                        else 201.. (UDP, below)
6   ParserCondition    not TCP                                        true 7-9: route by dst octet
10  ParserCondition    TCP fits                                       else 198-200 drop
11  ParserExtraction   TCP (20 B)
12  If                 DEVICE != 0, i.e. from a client                else 188.. (from the server)
13  If                 not SYN
    true  (client, non-SYN):
      14  BloomFilterQuery   flow in the bloom filter?
      15  If                 not in it
          true : 16..87   the cookie-check chain (12 rounds, 174 ops)         <- path B
                 88, 89   vector_borrow/return: the timesync delta            <- misplaced, see B
                 333-345  ctime = (ticks - delta) >> 12; cookie_val; age
                 90       If  age <= 2
                          true : 91 checksum, 92-94 return chunks, 95 fwd(0)  -> to the server
                          false: 96-99 drop
          false: 100-103 fwd(0)                                       -> to the server, untouched
    false (client SYN):
      104 If                 not ACK
          true : 105, 106  vector_borrow/return: the delta            <- already first
                 107..178  the SYN-ACK chain (12 rounds, 173 ops)             <- path A
                 179 checksum, 180-182 return chunks, 183 fwd(DEVICE) -> back out the ingress port
          false: 184-187 drop                                         (SYN-ACK from a client)
188 If                 not ECE   (server)
    true : 189-192 route by dst octet
    false: 193 bf_set, 194-197 drop
201 ParserCondition    UDP fits;  202 ParserExtraction UDP (8 B)
203 If                 from the server;  204 If dport == 5555;  205 If payload present
    210 ParserExtraction (4 B clock), 211 vector_borrow, 458 op_sub, 212 vector_return, 213-217 drop
    otherwise 206-209 / 218-228 route by dst octet
```

Two facts about the BDD's own order that the ground truth depends on:

- On path A the delta is read **before** the chain (105, 106 come first). On path B it is read
  **after** it (88, 89 follow 174 ops), because the C code hashes first and computes `cookie_time`
  only to compare. The ground truth reads it before the chain on both paths.
- Both chains have 72 `rotate_left` nodes, six per round, so the round boundaries are exact:

| after round | path A (SYN-ACK) | path B (cookie check) |
|---|---|---|
| 2 | node 118 | node 27 |
| 6 | node 142 | node 51 |
| 10 | node 166 | node 75 |
| 12 | node 178 | node 87 |

## The ground truth's shape, in these terms

Four passes with hash work, one recirculation, both client paths identical in shape:

| pass | gress | rounds | ends with |
|---|---|---|---|
| 1 | ingress | 1-2 | `SendToEgress` |
| 1 | egress | 3-6 | `Recirculate` |
| 2 | ingress | 7-10 | `SendToEgress` |
| 2 | egress | 11-12, then the tail | `Forward` (A) or `Forward`/`Drop` (B) |

Everything stateful -- device lookup, clock, delta read, bloom filter, the branch structure -- is in
ingress pass 1, before the chain. Nothing goes to the controller on a client path.

## The decision sequence

### Prologue, shared by every packet

| node | take | also offered (expected) |
|---|---|---|
| 2, 4, 11 | `ParserExtraction` | -- |
| 3, 5, 6, 10 | `ParserCondition` | -- |
| 12, 13 | `If` | `SendToController` |
| 14 | `BloomFilterQuery` | `SendToController` |
| 15, 104 | `If` | `SendToController` |

No cut before here: the crossing needs the pass to hold work first (`MIN_MODULES_BEFORE_EGRESS_CROSSING`),
and the ground truth crosses only after two rounds anyway.

### Path A -- client SYN, answered with a SYN-ACK  (13 false, 104 true)

| node(s) | take | note |
|---|---|---|
| 105 | `VectorTableLookup` (delta) | offered alongside `SendToController`; the read is already ahead of the chain |
| 106 | consumed by the lookup | |
| 107 .. 118 | `RotateLeft` / `ArithmeticOp` | rounds 1-2. At each rotate `RotateLeftShifts` is also offered: take `RotateLeft`. `Recirculate` and `SendToController` are offered throughout: never |
| **after 118** | **`SendToEgress`** | **decision 1.** Expect it offered: every way forward from here reaches route 183 without a data-structure call. The module emits the pulled-back forward (183: back out the ingress port) |
| 119 .. 142 | compute in egress | rounds 3-6 |
| **after 142** | **`Recirculate`** | **decision 2.** From egress |
| 143 .. 166 | compute in ingress, lap 2 | rounds 7-10 |
| **after 166** | **`SendToEgress`** | **decision 3.** Offered at 166 in the search log; whether it is offered *after a recirculation* is what the walk checks |
| 167 .. 178 | compute in egress, lap 2 | rounds 11-12 |
| 447 .. 457 | `ArithmeticOp` | the final xors: cookie = ctime ^ hash |
| 179 | checksum | `Ignore` today. Has to become a deparser checksum (Phase 3c); the walk takes `Ignore` and notes it |
| 180, 181 | `ModifyHeader` | address and port swap, seq = cookie, ack = seq + 1 |
| 182 | `ModifyHeader` / `Ignore` | ethernet chunk returned unchanged |
| 183 | `Forward` | nothing left to do: the port was written at decision 1 |

### Path B -- client ACK, cookie checked  (13 true, 15 true)

| node(s) | take | note |
|---|---|---|
| **88, 89 first** | `VectorTableLookup` (delta) | **reordering.** The BDD has them after 87. They depend on nothing in the chain, and the chain does not depend on them until 333, so lifting them to before 16 is legal on this path. Without it every way forward from any cut point hits 88 and the crossing is refused |
| 16 .. 27 | `RotateLeft` / `ArithmeticOp` | rounds 1-2 |
| **after 27** | **`SendToEgress`** | **decision 1.** Every way forward now reaches 95 (forward to the server) or 99 (drop); drop downstream is allowed, so the routes agree on the server |
| 28 .. 51 | compute in egress | rounds 3-6 |
| **after 51** | **`Recirculate`** | **decision 2** |
| 52 .. 75 | compute in ingress, lap 2 | rounds 7-10. **The walk stops at 58** (round 8): no hash-distribution unit left in any stage past 1, see below |
| **after 75** | **`SendToEgress`** | **decision 3** |
| 76 .. 87 | compute in egress, lap 2 | rounds 11-12 |
| 333 .. 345 | `ArithmeticOp` | ctime, cookie_val, age |
| 90 | `If` | `age <= 2`: a 32-bit inequality against a constant. `If.cpp`'s wide-constant path should keep it out of a gateway; the ground truth uses `const entries`. Take `If`, not `SendToController` |
| 91 | checksum | `Ignore` today, as on A |
| 92, 93 | `ModifyHeader` | seq - 1, ECE set |
| 95 / 99 | `Forward` / `Drop` | port written at decision 1; `Drop` in egress is the egress drop control |

### The other paths, all short and all in ingress pass 1

| path | nodes | take |
|---|---|---|
| bloom hit | 15 false, 100-103 | `ModifyHeader` x3 (unchanged), `Forward` (server) |
| SYN-ACK from a client | 104 false, 184-187 | `Drop` |
| server, not ECE | 188 true, 189-192 | `Forward` by dst octet |
| server, ECE | 188 false, 193-197 | `BloomFilterSet`, `Drop` |
| clock update | 201-205, 210-217 | `SendToController` is acceptable here: 3 packets in the profile, and the ground truth's in-switch version needs a register write the walk does not have to reproduce. Noted as difference 3 in `PLAN.md` |
| other UDP / non-TCP | 206-209, 218-228, 7-9 | `Forward` by dst octet |
| not IPv4 / too short | 229-230, 198-200 | `Drop` |

## Where the walk is expected to stop, and what each stop means

In the order the walk will meet them.

1. **Path B, before node 16: is the reordered delta read offered?** The reorderer runs inside the
   factories (`implement(..., !no_reorder, ...)`). If no candidate at the top of path B is a
   `VectorTableLookup` of node 88, the reorderer does not consider that lift and needs to. This is
   the blocker `PLAN.md` item 2a exists for.
2. **Decision 1 on either path: is `SendToEgress` offered?** With the read lifted it should be, by
   the guard's own rule (a clean way to a route exists). If it is not, the reason is in
   `SendToEgress.cpp`'s guard and is the next change.
3. **Decision 1: does the placer accept it?** The pass then holds the prologue plus two rounds.
4. **Decision 2: `Recirculate` from the egress.** Handled since `ee73c5ada`; verify.
5. **Decision 3: `SendToEgress` on the recirculated lap.** Offered at these nodes in the search log,
   but that log aggregates every EP that visited them; the walk settles whether it is offered after
   a `Recirculate` specifically.
6. **Anywhere: a dead end.** Every prefix should be completable by handing the rest to the
   controller, so a dead end is a bug. The one on record was a hoisted route with work left behind
   it. The walk reproduces it deterministically and reports the state.
7. **Nodes 91 / 179: the checksum call.** `Ignore` is the only option today; the walk records it
   and moves on. Not a reachability blocker; a correctness item for Phase 3.

## What the walk found (2026-09-12)

Against the list above, with `sc.txt` (318 picks) as the record:

1. Offered, one step late: not at 15 (the `If` child's active leaf is the else side), but at 16,
   where 88 is enumerated and `RotateLeft next=88 [R]` is offered. The delta is then read as a
   register on both paths (`VectorRegisterLookup` at 105 and at 88, `next=89 [R]`); the table form
   is refused on path B only because path A already made it a register.
2. Offered on both paths, after 118 and after 27.
3. Accepted on both.
4. Verified on both, after 142 and after 51.
5. Verified on path A, after 166. Path B never got there.
6. No dead end met.
7. Nodes 179 and 91: `Ignore`, as expected (91 not reached).

And one stop that was not on the list. **Path B, node 58, round 8 of lap-2 ingress: no compute
module is offered.** The compute step builder's reason: every stage from 2 on has no
hash-distribution unit left. A 32-bit hash rotate takes 2 of a stage's 6; each chain has 36
(three per round), 72 units; the BDD has the chain twice -- 16..87 and 107..178 are the same ops
on the same packet bytes -- so the two copies want 144 of the pipeline's 120. The ground truth
places the chain once per pass and runs it for a SYN and a bloom-miss ACK alike (`triage`,
`recirc_state`). bf-p4c shares hash units between mutually exclusive copies only when they are
the same computation on the same fields (`tofino/exp-compute/hdu1..4.p4`), which synapse's
per-node symbols are not. `PLAN.md` Phase 2, item 2a.

## What success looks like

A decision file that replays, non-interactively and under any heuristic, to a finished plan whose
emitted P4 has the shape in the table above: two crossings, one recirculation, 2/4/4/2 rounds,
no controller hand-off on 13-true or 13-false, both chains' state carried in the recirculation
header. Whether that P4 then compiles is Phase 3's question, not this one's.

That file is also the regression fixture: any later change that makes the replay stop short has
made the ground truth unreachable again, and says exactly where.
