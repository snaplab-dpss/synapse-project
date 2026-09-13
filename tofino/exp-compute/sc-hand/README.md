# By hand, from the walk's program to one that builds

Question: regen 23 of the walk (`../sc-walk/sc-walk.p4`) passes bf-p4c's PHV allocation and
action constraints but its table placement needs 32 stages. What does the program have to look
like to fit Tofino 2's 20, and which of that is the emitter's, the planner's or the search's to
change? Each file here is the previous one plus one edit, applied by script from the file's
statements and checked with `../tools/rotcheck.py` before compiling; the compile's outputs are
not kept. The ground truth's own build (`../tools/budget.py` on it) is the budget the edits
walk towards: 40 hash statements, 8 state words, 42 hash tables, 76 hash-distribution units.

| file | edit | bf-p4c |
|---|---|---|
| `sc-m1` | each lap-1 chain called once: the ingress's hoisted under a flag both branches set, the egress's after the code-path ladder | 37 stages: the hoisted egress chain serializes behind the code-path-4 branch, whose tables write the same words (two separate `if`s are not exclusive to bf-p4c) |
| `sc-m1b` | the egress chain as the first arm of the ladder, `(code_path == 0 \|\| code_path == 3)`, path 0's extra add nested | 23 stages (from 32): 27 actions had two call sites, and bf-p4c makes a table, with its own hash units, per call site |
| `sc-m1c`, `sc-m1d` | `rotate_left_144_x`'s hash xor, a path-B op whose action path A calls for the ops it shares, moved off `s32_0`, the word holding path A's loaded ack at that stage; `m1c` takes `s32_8` (one rotation conflict), `m1d` `s32_9` (none) | both 23 stages; a correctness fix: the ack-minus-one read the xor |
| `sc-m2`, `sc-m2b`, `sc-m2c`, `sc-m2d` | the packet-word hash entries as ALU statements (`m2d` with no rotation conflicts) | PHV allocation fails: the words' ALU copies into `meta.key_*` and the device id drag the keys into the sliced cluster |
| `sc-m3` | bloom keys hashed from the header slices; a faulty edit left slice self-copies | PHV allocation fails |
| `sc-m4` | keys substituted inside `.get({...})` only, the ack entries back in the hash unit, the device id through the hash unit | PHV passes; "no usable split format" for the bloom's register tables |
| `sc-m4b` | each bloom hash inside its register `execute()`, as the ground truth's `bf_row_0_read.execute(bf_hash_0.get({...}))`, declarations moved above their uses | 21 stages; 189 tables, 123 hash-unit uses |
| `sc-m5` | the egress lap-1 chain's five-action prefix (the tail of the round the crossing cut, and the message xor) moved to the end of the ingress lap 1 | the bloom's register tables cannot be placed; four rotation conflicts in the ingress. Not pursued |
| `sc-m6` | the four hash statements touching no state word as ALU statements, and the device-id copies on the ALU | PHV fails again: the bloom hash key ties `data4` to `data3`, which the chain reads on the ALU |
| `sc-m7` | as `m6` with the device-id copies back in the hash unit | 21 stages, 115 hash-unit uses |
| `sc-m8` | path A's `compute_op_add_243` call removed: its two ops' outputs alias the shared chain's first step (358, 359), which path A also runs for the ports-word load that action holds, so the step ran twice on path A | 21 stages; the ingress now ends at 19, one egress table at 20 |
| `sc-m9` | path B's cookie xor chain (453 -> 454 -> 455: three hash ops, three levels, six units) as one hash op over its four words, as the ground truth writes its cookie | **builds** (20 stages) |
| `sc-m10` | the ground truth's deparser checksums (PLAN 3d): IPv4 ttl/proto/checksum and TCP checksum/urgent words split into whole fields (a deparser `Checksum` can neither read nor write a slice), `meta.redo_checksum` + `meta.tcp_len` set by the SYN-ACK rewrite arm, both `update`s in the **ingress** deparser, since that packet leaves the ingress with the egress bypassed | builds (20 stages); the SYN-ACK is exact, checksums included |
| `sc-m11` | the controller hand-off: the seven chain values the emitter skipped (their homes are `hdr.st` fields) as `@in_hash` copies into seven new `hdr.cpu` fields in the controller's order; path B's egress tail (ops 333, 335) in two actions of its own instead of path A's `161_x`/`162`, whose other statements overwrite `s32_0/1/2/4` while path B still reads them | 22 stages: the seven hash copies chain one per stage, the hash-distribution units being spent |
| `sc-m11b` | as `m11` with the seven copies on the ALU | PHV allocation fails: the copies pull the cpu fields into the chain's sliced cluster |
| `sc-m11c` | as `m11` with no copies at all: `fwd_to_cpu` keeps `hdr.st` valid, so the state header follows the cpu header to the controller (and `parse_cpu` extracts it on the way back); the controller's `cpu_hdr_extra_t` (`sc-m11c.cpp`) names the state words by the values they hold at the hand-off | builds (20 stages); the controller gets its 121 bytes and drops the ACK: the words it reads are not the reference's |
| `sc-m12` | path B's second lap (the `code_path == 0` arm) called path A's `144_x` / `144` for the three round steps it shares with them; those actions also carry A's `v3 ^= seq`, `rotl(v3, 8)` and `v2 += v3`, which read A's `v3` (in `s32_9`, from `hdr2.data1`) where B's is `seq - 1` (in `s32_2`), so B's `v2` was added to twice. B now calls two actions holding only its share | builds; path B's words match the reference through the hand-off, path A's cookie is still wrong |
| `sc-m13` | the SYN path's final round: `177` wrote `rotl(v3, 7)` into `s32_0` over `rotl(v1, 13)`, which the exit xor (`453`, folded into `455` since `m9`) reads after it; regen 23 had the same overwrite. `rotl(v3, 7)` takes the free `s32_2`, `456` reads it there | builds (20 stages); scenarios 1-4 pass; 5 fails only because the BDD was built with `--sip-key0 857870640` (0x33221130, a decimal typo of upstream's 0x33323130 in `dpdk-nfs/smartcookie/Makefile`), so the test's reference hash used another key: `SC_SIP_KEY0=0x33221130` (a switch added to the test) |
| `sc-m14` | the parser: `hdr3` (UDP) selects on the destination port, 5555 extracting the 4-byte clock (`hdr4`); the emitter had left BDD 204 (`ParserCondition`) and 210 (`ParserExtraction`) out of the parser, since they sit below the ingress `if` on the device (203) and the parser leaf they belong to was already terminated | builds; the clock update reaches the controller, which sets the delta from the host clock |
| `sc-m15` | the controller's `now` for a packet is the switch's: `hdr.cpu.time = meta.time` at the hand-off, `cpu_hdr_extra->time` in the controller (`sc-m15.cpp`) where it had `(now>>16) & 65535`, the host's clock and a 16-bit mask of a 32-bit read | **builds (20 stages) and passes all 13 scenarios** |

The model test (`tests/smartcookie.py --up --nf smartcookie-walk`, with the synthesized topology
`SC_SERVER_PORT=1 SC_SERVER_DEV=0` and the BDD's key `SC_SIP_KEY0=0x33221130`) is what found the edits
after `m9`; `sc-m15.p4` with `sc-m15.cpp` passes all of it (2026-09-13); `synthesized/smartcookie-walk.*`
is the file under test (regen 23's controller and JSON, the P4 of the hand version). What it found,
in order: a container with a stale libsycon (no edit; the bloom cleanup thread never stopped and
the port table stayed empty); the SYN-ACK carrying the SYN's TCP checksum (`m10`); the controller
asserting on a 77-byte packet whose CPU header held 3 of the 10 values it reads (`m11`), and,
while mapping those values to their words, path B's egress tail clobbering four of them (`m11`).
Then, with `../tools/siptrace.py` matching the state words the parser extracts on every pass
against a reference HalfSipHash trace: path B's second lap reading path A's `v3` (`m12`), the
SYN path's exit xor reading an overwritten word (`m13`), the BDD's key (a Makefile typo), the clock
update never parsed (`m14`), and the controller's delta taken on the host's clock (`m15`).

Takeaways, each a change synapse now has to make, in the order the evidence came:
1. A chain shared by two paths is emitted at one call site, inside the same exclusive
   ladder as the other laps; bf-p4c makes a table per call site.
2. A foreign op carried by a shared action must not write a word live on the calling path at
   the call; the planner's guard has to hold whichever path is planned first.
3. A packet word may be read on the ALU next to the chain only if nothing else reads it on the
   ALU (the ground truth's `data5`, `data6`, `ports`); keys are hashed from the header slices in
   the `get` call, and an ALU derivation from such a word (the device id) goes through the hash
   unit or a table.
4. A bloom hash is computed inside the register's `execute`.
5. Hash statements touching no state word are ALU statements.
6. An op whose outputs alias a shared step the path runs anyway is not emitted on that path.
7. A chain of xors leaving the chain is one hash op.
8. The hand-off to the controller sends every value the controller reads, header-slot homes
   included: the emitter skips header fields on the assumption that the controller reads them from
   the packet, which holds for packet headers and not for the state header. Copying the words costs
   what the chain cannot spare (hash units or the sliced cluster); the state header itself can go.
9. An action two paths share carries only statements both paths run; a path's own ops get their
   own action.
10. The deparser checksums, as PLAN 3d describes and `m10` does.
11. A value read by a later hash op keeps its word until that op runs: the planner let `rotl(v3, 7)`
    take the word of `rotl(v1, 13)` before the exit xor read it (both `@in_hash` writes, same
    action pair, every regen since the header slots).
12. The BDD's SipHash key is the Makefile's, in decimal: keep the decimal right, and let the test
    take the key from the environment for a synthesized solution.
13. A parser condition or extraction below a non-parser node still goes into the parser: the
    packet-field conditions select, the others (the device here) are re-checked in the ingress.
14. `now` on the controller is the switch's timestamp of the packet, carried in the CPU header;
    the host's clock is another clock.
15. The controller transpiler's narrow read of a shifted symbol masks with the shift's width
    (`(now>>16) & 65535` for a 32-bit read), and `(1 << size) - 1` overflows at 32.
The stage count was hash-distribution units first (6 a stage, shared by both gresses' passes,
every stage full) and the egress lap's chain depth second (four levels a round on the v3 lane,
as in the ground truth, plus the prefix a mid-round crossing leaves).
