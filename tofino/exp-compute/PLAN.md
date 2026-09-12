# Plan: a SmartCookie that synapse finds, that compiles, and that passes the model test

*2026-09-12, revised the same day after discussion. Companion to `GROUND-TRUTH.md`, which this
both relies on and corrects in places; that file is evidence, not gospel. The ground truth written
as a sequence of synapse decisions is `GT-DECISIONS.md`.*

## Where we are

`synthesized/smartcookie-unrolled.p4` is the target: it compiles for Tofino 2 in 7 s (18 ingress /
19 egress stages, 121 tables) and passes `tests/smartcookie.py` 13/13, re-verified on 2026-09-11
from a fresh build.

Synapse's current output for the same NF (`HEAD`, `max-tput`, seed 0, profile
`smartcookie-f40000-c0-unif`) is 286.83 Mpps, 1 recirculation, `Stages: 20`, and **does not
compile**: bf-p4c grinds in PHV allocation until the 5-minute cap. That output is what this plan
starts from.

Everything below is grounded in measurements on that output and on the target; where something is
inferred rather than measured it says so.

## The differences, and which of them matter

Measured on today's output against the target.

| # | synapse | target | matters |
|---|---|---|---|
| 1 | **cookie verification runs on the controller**: the client-ACK / bloom-miss path (9.85 % of profile traffic) hands off mid-chain at BDD 88, the timesync-delta `vector_borrow`, after ~2 rounds in-switch; the remaining rounds, the age check (BDD 90, `cookie_time - cookie_val <= 2`) and the forward/drop run in software at the configured 100 pps | all in-switch | **yes** -- functional under load |
| 2 | SYN -> SYN-ACK path (39 %): in the data plane, crosses to egress, recirculates once | same outline | no |
| 3 | clock update (3 packets): controller | in-switch | no, cold; accepted as a divergence |
| 4 | **three hash passes**; the egress runs one unconditional block | four; egress dispatches on `code_path` and runs on both laps | **yes** |
| 5 | **rounds per pass ~4 / ~2-3 / ~1-2** | 2 / 4 / 4 / 2 | **yes** -- the PHV pressure |
| 6 | routing decided after the chain (`Forward` at the end) | before it: `triage` picks callback and egress port, carried in the state header | yes -- it is what makes the crossing legal |
| 7 | delta read sits mid-chain in the BDD on the cookie-check path (87 `rotate_left` -> 88 `vector_borrow` -> 89 `vector_return`); it is the hand-off point | delta read before the chain starts on both paths | **yes** -- the reason for #1 |
| 8 | 2 controller hand-offs | 0 | follows from 1, 3 |
| 9 | self-reported stages 20 | actual 18 / 19 | to verify (see the model section) |
| 10 | **one 32-bit metadata variable per op**: 197 ingress (128 rotate temporaries, 27 add, 23 xor, ...), 81 egress | 9 / 10: `v0..v3` in the `recirc_state` header, `a0..a3` temps | **yes** |
| 11 | carried across the lap: recirc 320 b (10 fields) + egress_state 448 b (14) + cpu 488 b | recirc 64 b + recirc_state 184 b | yes |
| 12 | 7 carried fields never read (5 egress_state, 2 recirc) | 0 | yes, cheap |
| 13 | `@in_hash` 70 / 26 | 20 / 20 | consequence of 10 |
| 14 | actions 100 / 34 | 79 / 54 | consequence of 5 |
| 15 | tables 3 / 0; branching by if-chains | 6 / 3; `const entries` tables for triage, message and final selection, `cookie_age` | partly: `age <= 2` is a 32-bit inequality the target cannot put in a gateway; synapse never met it because its age check is on the controller |
| 16 | bloom rows: read and set as separate bare actions, each its own keyless table | one table per row, both actions keyed | latent: bf-p4c can place them in different stages and split the register |
| 17 | **no checksums** (`Ignore.cpp` drops the call) | IPv4 + TCP recomputed in the deparser | **yes** -- the test's SYN-ACK case |
| 18 | a temporary written and read in one action (`hdr_val0/1/2`) | none | yes |
| 19 | 8 nested plain ALU writes of chain values into deparsed headers | final writes through `@in_hash` | yes |
| 20 | header layout guessed and coalesced (IPv4 as 5 x 32) | hand-declared, checksum fields split out | with 17 |
| 21 | 55 / 24 rotate sites incl. shift-form | 36 / 37 concat-form | consequence |
| 22 | recirculated lap emitted first | same | no |

Correction to `GROUND-TRUTH.md`: the target's rounds per pass are **2 / 4 / 4 / 2**, not 2 / 4 / 2 / 4.

## The question that comes first: is the target reachable at all?

Three layers decide what synapse emits, and the plan attacks them in this order, not the order
they appear in the code:

- **offered** -- at each step, does some factory produce the child the target needs?
- **placeable** -- does the placer accept it?
- **preferred** -- does the heuristic pop it?

Reachability is *offered and placeable*, with *preferred* taken out of the picture entirely. That
is what a walk in which a human picks the child at every multi-child step isolates: any failure to
follow the target's path is then an offer-side or placer-side fact, and a concrete synapse change.
`GT-DECISIONS.md` is the path to walk.

What the walk is expected to find, from what is already known:

- **The delta read on the cookie-check path** (difference 7). The crossing's guard
  (`SendToEgress.cpp`) refuses to cross while every way forward hits a data-structure call. Its
  reason is not a placement rule: *the egress backend emits compute and nothing else*, so a
  structure past the cut would be stranded. That is an emitter limitation. The general fix is to let
  the egress emit tables and registers, with the guard then refusing only when the structure is
  *also* touched in ingress; for SmartCookie the delta vector is written in ingress on the clock
  path (node 212), so the read has to stay in ingress anyway, and lifting it ahead of the chain --
  which is what the target does -- is the fix to try first. The BDD already has it first on the
  SYN-ACK path (105, 106); on the cookie-check path it sits after the chain (88, 89) and depends on
  nothing in it.
- **The second crossing.** Withdrawn as a blocker: the search log shows `SendToEgress` offered at
  the lap-2 nodes (166, 168, 171, 173, 175, 176, 178, 456), the first few excluded only by the
  "pass already holds work" threshold. The walk confirms it holds after a `Recirculate` specifically.
- **A dead end, if one appears, is a bug.** Any prefix should be completable by handing the rest to
  the controller, so the search should never corner itself; `--allow-deadends` exists so users can
  get past such bugs, and is not to be used here. The one on record (`GROUND-TRUTH.md`, step 113):
  BDD reordering hoisted the route, `Forward` was placed, work was left behind, and once a
  forwarding decision exists `SendToController` is no longer legal -- the escape hatch is closed by
  the very state the reorderer created. To discuss when the walk reproduces it, with the exact
  state in hand.

## The plan

Ordered by what has to be true before the next thing can be asked.

### Phase 0 -- the target as a decision sequence  *(done: `GT-DECISIONS.md`)*

Node by node, module by module, with the three cut points named by BDD node (118 / 142 / 166 on
the SYN-ACK path, 27 / 51 / 75 on the cookie check), the reordering the cookie-check path needs,
and the seven places the walk is expected to stop.

### Phase 1 -- the walk: a replayable, interactive decision layer

A debugging instrument, so it must work under any heuristic. Two things already exist and are
used as they are:

- **`--backtrack` is strict mode.** It stops the search on the first backtrack and dumps the BDD,
  the plan and the search space. A pick that dies is exactly a backtrack, so nothing is built for it.
- **The search-space dump (`-ss.dot`) is the record of what was offered.** Every child generated at
  every step is in it, with its score and an `[R]` mark on reordered ones. Three of this plan's open
  questions were answered from one morning's dump without touching synapse (below).

What the walk adds, and nothing more:

- **1a. The choice.** In the search loop (`Search.cpp`), after a step's children are generated and
  before they reach the heuristic: at **every** multi-child step, print each child as
  `module . BDD node . [R: which node it moved] . score`, prompt, and add the pick's EP id to the
  forced-decision set. That set is `values[0]` of the score every heuristic shares (all of them
  derive from `HeuristicCfg`), so the pick's subtree is explored first under any heuristic, the
  siblings stay in the open set below it, and a pick that dies backtracks to the most recent
  alternative. Absolute control; no auto-follow.
- **1b. The decision file.** One line per choice, keyed by the BDD node processed, the module, and
  the node that comes next in the chosen plan -- never by EP id, which depends on generation order.
  Written as choices are made; replayed non-interactively; on a step the file does not cover, stop
  and print the offered set. The target's file is then the regression fixture: a change that stops
  it replaying to completion has made the target unreachable again, and says where.
- **1c. Decline reasons.** A factory that produces nothing leaves no node in the search space, so
  this is the one thing the dump cannot show. First cut: `SendToEgress`, `Recirculate`,
  `SendToController`, `VectorTableLookup`, `BloomFilterQuery` say why they declined, and
  `get_reordered` prints the candidates it enumerated at the anchor.

#### What the search space already showed (2026-09-12)

- **A. The delta-read lift is never offered at node 15.** The `If` child's active leaf is the
  *else* side (`Next: 100`), so reordering is attempted around {15, false}, where everything ahead
  is `packet_return_chunk` and a route -- unmovable. The anchor {15, true}, where 88 is a Valid
  candidate (checked with `bdd-reorder`, and 89 chains after it), is never the reorder anchor.
- **A'. Nor at node 16, for a reason only the running search can show.** The search materialised
  nine reordered children there and the tool enumerates nine at the same anchor -- one element
  different: the search moved 17 where the tool has 88. Same call, same flags, on the plan's own
  BDD copy. 1c's enumeration line resolves this on the first walk.
- **B. `VectorTableLookup` declines node 88, always.** In natural order after the chain, and when a
  plan reached 88 early by a lift (EP 1287, progress 208, expanded to `Recirculate`, seven
  `ArithmeticOp`, `SendToController`). The identical read at node 105 on the SYN path is accepted.
  The guards are `is_vector_read`, `can_impl_ds(obj, Tofino_VectorTable)` and table placement;
  1c says which.

So the cookie-check path is blocked on the offer side at two independent points, neither of them
the crossing guard first blamed.

#### What the walk showed (2026-09-12; decision file `sc.txt`, 318 picks)

- **Path A replays to the end in the target's shape:** register read of the delta at 105, cross
  after 118, recirculate after 142, cross after 166, tail to `Forward` at 183.
- **A' is resolved.** At node 16 the reorderer enumerates 88 and offers `RotateLeft next=88 [R]`.
  The omission at 15 (A) stands but costs nothing: the lift is offered one step later.
- **B is explained, and is not a bug.** `VectorTableLookup: the vector is already committed to
  another implementation (can_impl_ds refused Tofino_VectorTable)`: the delta became a register on
  path A, so path B has to read it as one too, and `VectorRegisterLookup` is offered at 88 with
  `next=89 [R]`. The two paths must agree on the delta's data structure; they do.
- **Path B replays decision 1 (cross after 27) and decision 2 (recirculate after 51).**
- **C. Path B stops at node 58 -- round 8, lap-2 ingress -- on the placer.** No compute module is
  offered. The reason, once 1c reached the compute step builder:
  `no stage takes rotate_left_58_x (alu ...): Unsat; deps: compute_rotate_left_55@19/ingress ...;
  soonest stage -1; free hash-dist units by stage: 2 6 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0`.
  Every stage from 2 on has no hash-distribution unit left, so round 7's hash rotates were pushed
  to the last stages that had any, and round 8 has nowhere to go. The arithmetic: a 32-bit hash
  rotate takes 2 of a stage's 6 units; the chain has 36 of them (three per round), 72 units; the
  BDD has the chain **twice** -- 16..87 and 107..178 are the same ops on the same packet bytes
  (16 and 107 are both `rotate_left(740644437, 5)`, 17 and 108 both xor the same four bytes) --
  so two copies want 144 of the pipeline's 120. Path A's copy took 72; path B's ran out at round 8.
  The ground truth places the chain **once** per pass: `triage` sets `recirc_state` for a SYN and
  for a bloom-miss ACK alike, and `i1..i2`, `e3..e6`, `i7..i10`, `e11..e12` run for both.
  Whether bf-p4c would let two copies share was tested (`hdu1..4.p4`, README): two `if`/`else`
  copies of the same hash rotates on the *same fields* share the stage's units (both copies' tables
  use `hash_dist(0,1)/(2,3)/(4,5)`, 12 stages); the same ops on *different* fields, or different
  rotates, do not, and the second copy is serialized past stage 20. Synapse's chains are the second
  case: every node has its own symbol (`rotated__16` vs `rotated__107`), so the emitted copies
  would read different fields even if the placer let them through.

So the target is unreachable today for a reason that is neither an offer nor a guard: **the
pipeline holds one chain, and synapse builds two.** The model is right on this point -- the toy
fails the way the placer says -- and what is missing is the ground truth's sharing.

### Phase 2 -- make the target offered and placeable

Fix what the walk stops on, in the order it stops. The walk found one stop, C, and it is the
whole of this phase until a re-walk finds the next.

- **2a. Share identical stateless compute across mutually exclusive paths.** The two chains are
  the same ops on the same inputs, and bf-p4c shares hash units only for the same computation on
  the same fields, so synapse has to produce exactly that. Proposed: value-number the stateless op
  nodes (`rotate_left`, `op_*`) over the BDD, so that ops equal up to the renaming of the symbols
  earlier equal ops produced get the same `ComputeAction` id. The placer then treats the second copy
  as already placed -- as it does for a table reused on another path (`already_requested`) -- and
  charges nothing; the emitter emits the action once, applies it on both paths, and aliases the
  second path's symbols to the first's. Two things to check on the way: a shared action has to sit
  after *both* paths' prologues (path A's chain follows 104-106, path B's 14-15; the pipeline's
  re-placement of an already-requested structure with new dependencies is the mechanism), and
  logical ids, since a shared op is still one table per path in a stage (six rotate tables per
  stage compiled in `hdu2`).
  Rejected: hoisting the chain above 13 with the reorderer's sibling merge. It merges the copies,
  but puts the chain -- and its recirculation -- on the bloom-hit path (15 false), which the target
  forwards in one pass.
- Then re-walk. Expected next: nothing before the tail of path B (333-345, 90, 92-95/99) has been
  reached yet, so its stops are still predictions.

**2a is built (uncommitted, 2026-09-12).** The registry lives in the Tofino context, keyed by the
op's function and operands with reused symbols substituted; the step builder queries it before
asking for a stage and reuses an action placed in the same gress at or after the op's producers;
the emitter binds a reused op's output to the original's variable and calls the shared action
from the second branch. Two things came out of the first replay:

- **The pipeline's gress was a global flag.** It is set by the last `SendToEgress` or
  `Recirculate` placed, on whatever path that was, so when the search moved from path A's tail
  (in the egress) to path B's first node (lap-1 ingress) it still said egress: reuse was refused
  as a gress mismatch, compute ops were charged to the wrong gress, and dependencies in the ingress
  counted as already satisfied. Fixed: the search syncs the gress to the active leaf's position on
  its own path before every step (`Context::sync_active_leaf`).
- **With that, lap 1 is shared** (155 ops reused on the walked path, hash units free again in most
  stages) and the walk gets past node 58, to **node 72**: rotate 70 sits in stage 19 and 72 has no
  stage. The reason is the lap-2 copies. Rounds 7-12 differ in one input (sequence number vs
  sequence number minus one), so path B's lap 2 is placed fresh, and stages 2-11 now hold five
  chains' hash rotates (shared lap 1 in both gresses, path A's lap 2 in both, path B's lap-2
  ingress) where the ground truth holds four. Path B's lap-2 hash rotates find no unit in those
  stages, land later, and the chain runs off the end. `hdu8` is the way out and the ground truth's:
  make the one differing input a shared field.
- Four other NFs (cl, fw, hyperloglog, psd) re-synthesize to byte-identical P4 and controller code
  with the change.

**2b is built as a rule of the placer, not a BDD pass (uncommitted, 2026-09-12 evening).** An op
with no exact match looks for a placed op of the same shape on another path -- same function,
operands equal but for one plain value -- and shares it once that value reads one field on both
paths: unified by naming when both paths compute it, by a move into the other path's field when
one reads it from the packet or as a constant (`hdu8`). The placed op's operand is rewritten to
the shared field; the move is a keyless action placed ahead of the shared one and emitted at the
start of the run that holds it. Speculation sees it through the same builder; it no longer
registers what it places, which is what made each replay slower. Found on the way and fixed: the
first cut matched ops on the *same* path (two live values cannot share a field), and a symbol
that reached a path by an alias is the other path's, not a candidate for unifying.

Result: **the walk replays to a finished plan of the target's shape** with no prompt
(`smartcookie-walk.txt`, 456 decisions, the regression fixture). Path B's lap 2 places one action
of its own (the `seq - 1`) plus the move on path A; rounds 7-12 share with 144, 403, 417 and their
neighbours. Clock path to the controller, as allowed. `Tput 286.83 Mpps, Recirculations 2,
Stages 20`. Four other NFs re-synthesize to byte-identical P4 and controller, but for
hyperloglog's header-value actions (below).

**Phase 3 has started with what compiling that plan found** (`tofino/exp-compute/sc-walk/`):
- a value carried by a recirculation must keep its header slot on every path that carries it, or
  a shared lap-2 action reads the wrong slot (slots are now remembered by variable name);
- the emitter's own common-subexpression trick (a value already held elsewhere gets no statement)
  must not apply to a shared action, which the other path calls for that very op;
- after a recirculation from the egress the clock is `meta.time` again, and a shift of the clock
  in the egress is `eg_md.time` (two pre-existing bugs, exposed by the target's shape);
- a materialized header value written as a bare assignment in the apply block makes bf-p4c
  synthesize an action and refuse the wide constant in it (`ack - 1`); it is now an action of its
  own (`addc1..3.p4`).
With those, the front end and instruction selection pass, and after fifteen minutes the back end
stops on action constraints: a rotate output the hash unit cut six ways
(`rotate_left_162_out[15:0]`, `[18:16]`, `[23:19]`, `[24]`, `[26:25]`, `[31:27]`) copied into
`hdr.egress_state` at the crossing needs six PHV sources against a limit of two, and
`build_recirc_hdr` packs `meta.dev[31:16]` with the two halves of `ingress_port`, three sources.
The PHV work the hand-fixed version needed (`GROUND-TRUTH.md` rules 8-10, the hash-unit copy for
cut values), which is this phase's subject.

The original proposal, kept for the record:

**2b (as first proposed): a selector for the one input that differs.** A BDD pass over stateless ops:
value-number them, match the two chains in lockstep (they are the same unrolled function), and at
the first op that differs in exactly one plain operand insert an `op_phi(k, x)` node on each path
producing a fresh symbol -- on path A `x` is the sequence number, on path B the expression
`seq - 1` that node 285 computes today, which the phi absorbs -- and rewrite the ops downstream to
read the phi's symbol. The Tofino side places a phi as one ALU move and, seeing another phi with
the same `k` already placed, aliases its output symbol to that one's instead of reusing the
action: the two moves write one field, and everything after them matches by the existing
registry. The phi reads only packet bytes, so it can be lifted ahead of the recirculation and
carried in the recirculation header, which keeps lap 2's depth where it is (path A's lap 2 already
ends in stage 19). The ground truth's `msg3_sel` table is this phi.

Removes differences 1, 4, 5, 6, 7, 8 -- as a *reachable* plan, not yet a chosen one.

### Phase 3 -- emitter: the reachable plan has to compile and behave

None of these changes the plan; they are what the replayed plan needs to pass the test.

- **3a. Liveness pooling across the gress** (the `fK` hand fix, 191 -> 31, never ported). Then, if
  still short, the chain state as `v0..v3` in the state header with four temps per gress, which is
  what gets the target to 9. (10, 11)
- **3b. Carry only what is read downstream** at the crossing and the recirculation. (12)
- **3c. Three emitter rules:** a temporary must not be written and read in one action (18); a
  chain value written into a deparsed header goes through the hash unit, nested sites included
  (19); a register's read and write share one table (16).
- **3d. Checksums** (17): stop dropping `nf_set_rte_ipv4_udptcp_checksum`, flag the path, emit
  the deparser `Checksum().update` guarded on it, split the checksum fields out in header guessing
  (20). The BDD names the IPv4 and L4 chunks by object id, so no protocol table is needed.
- **3e. Stage packet-field operands in the parser** (`tofino/experiments/parser-staging`): free.
- **3f. `age <= 2`** (15): once the age check lands in the data plane, confirm `If.cpp`'s
  wide-constant path keeps a 32-bit inequality out of a gateway.

Milestone: the replayed target compiles and passes `tests/smartcookie.py` with the synthesized
topology (`SC_SERVER_PORT` / `SC_SERVER_DEV` per `configs/tofino2-smartcookie.toml`).

### Phase 4 -- make the target preferred

Only now does the heuristic matter. Force the target's shape (its decision file) and read what the
oracle scores it; compare with the offloading plan's 286.83 Mpps. The oracle caps the controller at
100 pps, so offloading 10 % of traffic should score badly -- measured, not assumed. Then whatever
the cost model needs so that an unforced search picks it: at minimum, something that prefers a pass
boundary where few values are live, which nothing in the score expresses today.

### Phase 5 -- the pessimistic model  *(deferred by decision, to the end)*

Emitting a program that does not compile is a bug in its own right: the placement model is meant
to be dumber than bf-p4c, so that everything emitted compiles. The failure is PHV, the config
declares the pool (`phv_32bit_containers = 80`), and nothing draws on it -- `a04d6ffad` charged
computed values per pass, `05bca9e5e` reverted it because a per-pass reset models a constraint the
hardware lacks, and nothing replaced it. What bf-p4c actually runs out of, measured:

- **live 32-bit compute values per gress** (target <= 10; synapse ~4 rounds' worth, 23 even
  pooled), each carrying the chain's five rotate cuts that the allocator has to keep co-located;
- **carried header fields**, each a `deparsed exact_containers` container pinned for the life of
  the program (target 6, synapse 35).

A pessimistic model charges both against the per-gress pool, assumes no overlay between passes
unless the emitter guarantees reuse (3a), and sets the cap at a measured failure boundary less a
margin -- calibrated by adding rounds to the target until bf-p4c fails. `max_compute_ops_per_gress`
counts the wrong quantity and, enforced without a legal crossing, drove the search to the controller
and the search-space estimate to 8.6e71 (`tofino/experiments/compute-cap`). Possibly also: the
stage model reports 20 where bf-p4c has needed 21-25 on hand variants -- unverifiable until PHV
passes.

### Phase 6 -- validate, throughout

After every phase that touches shared emission or placement: regenerate the other six NFs, build
all 218, run the NF tests that exist.

## Next step

Phase 1 is built and has walked both client paths (`sc.txt`). Phase 2 starts with 2a, and 2a is a
design decision: value numbering in the BDD, `ComputeAction` ids from it, and symbol aliasing in
the emitter touch three layers. To agree before anything is written.

## Open questions

- ~~The reorderer: does it consider lifting a `vector_borrow`/`vector_return` pair ahead of a run of
  pure compute?~~ It does, at node 16 (A'); and the register form is what both paths take.
- Sharing (2a): is value numbering over the BDD the right level, or should the equivalence be
  found by the placer when it sees an op whose canonical form is already placed in this gress?
  The BDD level makes the emitter's aliasing a lookup; the placer level keeps the BDD untouched.
- The dead-end invariant: when a route is hoisted ahead of work, should the hoist be refused, or
  should a placed `Forward` stop closing the door on a later controller hand-off?
- Overlay across passes: does bf-p4c overlay metadata of mutually exclusive passes on its own?
  Decides whether Phase 5's charge is the peak or the sum, and whether 3a is needed for that.
- Is a live-value *count* the right proxy for Phase 5, or does the model need the cut set per
  value? The target's `v0` is sliced twelve ways in one container and is fine; "too many sources"
  says co-location, not count, is what runs out.
