# Tools behind the `scy-*` and `st*` experiments

Small scripts that produced or checked the toys in the parent directory. They read plain P4 or
the `[homes]` dump synapse prints in walk mode (`--walk`), and take their inputs as arguments.

| script | what it does |
|---|---|
| `rehome.py SRC DST PORT COMMENT` | re-homes a skeleton's call sequence in the ground truth's discipline (adds and hash rotates into pinned metadata temporaries, xors into header words, copies where a value is needed as both): `scy-gt*.p4`, `scy-b1-gt.p4` |
| `rehome2.py SRC DST PORT COMMENT [--hash-va] [--meta-a] [--pragmas]` | the same under strict alternation, every ALU statement reading one kind of word and writing the other: `scy-*-alt*.p4` |
| `rehome3.py SRC DST PORT COMMENT [--no-rule]` | one pool of words by live range, a word never written from another word at two rotations: `scy-*-one.p4`, `scy-b1-onefree.p4` |
| `rotcheck.py FILE.p4` | per gress, the (destination word, source word) pairs an ALU statement writes at more than one rotation |
| `cluster.py FILE.p4` | per gress, the 32-bit fields the chain's ALU statements tie together, against the budget of twelve |
| `realloc.py DUMP [heuristic...]` | replays synapse's slot allocation offline from a `[homes]` dump under several slot-choice heuristics |
| `exact2.py DUMP KI KE` | searches for an assignment of the plan's values to KI ingress and KE egress words that respects live ranges and the one-rotation rule (feasible at 11 and 11 on the SmartCookie plan, not found at 10) |
| `siptrace.py MODEL_LOG SRC SPORT DST DPORT [SEQ] [KEY0] [KEY1]` | matches the state words the ingress parser extracts on every pass of a model run against a reference HalfSipHash trace of the flow (lane, round, step); the first word that matches nothing is where a pass went wrong. Found `sc-hand`'s `m12`, `m13` and the BDD's key |

The skeleton the `scy-*` files share (`scy-all.p4`) holds regen 13's ingress compute actions
verbatim; `scy-pN.p4` cut its second-lap sequence to its first N actions.
