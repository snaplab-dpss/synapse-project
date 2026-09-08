# Ground-truth SmartCookie (`synthesized/smartcookie-manual.p4`)

A complete SmartCookie for Tofino 2, written by hand, that compiles **and passes the model test**
(`tests/smartcookie.py`). It is the reference the synthesized solution should be judged against:
what synapse ought to produce, and the yardstick for how far its output is from something the
target accepts.

It lives in `synthesized/` rather than here so it sits next to the machine-generated solutions it
is meant to be diffed against, and so the testbed can run it like any other NF:

```
bf-p4c --target tofino2 --arch t2na -o out synthesized/smartcookie-manual.p4   # 6 s
sudo -E tests/testbed.py up smartcookie-manual && sudo -E python3 tests/smartcookie.py
```

It follows synapse's own P4 template: the same `synapse_ingress_headers_t` and
`synapse_ingress_metadata_t` structs, the `cpu_h` / `recirc_h` / `recirc_state_h` headers, the
`hdr0..hdr4` layout that synapse's header guessing produces for ethernet, IPv4, TCP and UDP, the
`TofinoIngressParser` and `IngressParser` shape, the `fwd_op_t` enum, `ingress_port_to_nf_dev` and
the usual forwarding actions. A reader should be able to diff it against a synthesized file.

| | |
|---|---|
| stages | 17 ingress, 17 egress (of 20 each) |
| match tables | 47 |
| chain actions | 28 declarations (17 names, most written once per pipeline), executing 12 SipRounds |
| `@in_hash` sites | 10 |

## Semantics

Taken from `dpdk-nfs/smartcookie`, the NF synapse compiles, not from the paper:

- `cookie_hash` = HalfSipHash-2-4 over four words: source address, destination address,
  source port concatenated with destination port, and the sequence number.
- `cookie` = `cookie_time ^ cookie_hash`, with `cookie_time = (ticks(now) - stored_delta) >> 12`.
- A client SYN is answered with a SYN-ACK whose sequence number is the cookie and whose
  acknowledgement is the original sequence plus one, with addresses and ports swapped.
- A client non-SYN is passed to the server if the bloom filter already holds the flow, otherwise
  its cookie is checked: `cookie_val = (ack - 1) ^ hash(..., seq - 1)` and the packet is accepted
  only if `cookie_time - cookie_val` is 0, 1 or 2.
- An ECE-tagged packet from the server records its flow in the bloom filter and is dropped.
- Any other IPv4 packet is routed by the first octet of the destination address.
- A timesync packet from the server updates the stored delta.

## Structure

The twelve SipRounds are one body, written once per pipeline and executed twice per pipeline per
lap, two in ingress and two in egress. Twelve rounds therefore take three laps. The four state
words, the round counter, the callback type, the egress port and the cookie time travel in a
recirculated header; the message word for each lap is chosen by a table keyed on the counter.

This is the shape the whole investigation converged on, and each element of it was forced by a
measured failure. See `README.md` for the experiments behind them.

## What synapse cannot express today

Kept here so the gap is explicit. Ordered by how hard each looks.

1. **One body executed many times.** The chain is written once and re-executed on each lap, so
   three laps cost one lap of hardware. Synapse emits separate code per lap, because its BDD
   arrived from symbolic execution with the loop already unrolled and nothing marking the twelve
   rounds as iterations of one thing. This is the large gap: it needs the repeated structure
   recognised, a body emitted once, and the search and placer taught what that costs.
   *Consequence today: 92 distinct actions and about 160 live values, against 19 and 9 here.*

2. **Using the egress pipeline.** All synapse output lives in ingress. The ground truth needs
   egress for half the rounds, which is what brings the hash-unit demand inside budget. This is
   the smaller gap and it may matter more: the unrolled form has never been tried with the work
   split across both pipelines, and that experiment is still open.

3. **A loop counter and dispatch on it.** A field in the recirculated header, incremented each
   lap, with tables keyed on it selecting the message word and the finish action. Synapse has no
   module for a counter carried across recirculations, nor for dispatching on one.

4. **Emitting an operation into the hash unit deliberately.** `@in_hash` is used here for the
   non-byte-aligned rotates, for the four-way final xor, for the byte read that picks the routing
   port, and for the timestamp read. Synapse has no notion of choosing the hash unit for an
   operation that would otherwise not fit.

5. **Placing hash-producing actions to respect their own limits.** One `@in_hash` per action, one
   hash-producing action per table (two exceed the immediate pathway), and a hash operation
   cannot sit in a keyless table. These are emission rules, mechanical once known.

6. **Wide comparisons as table entries.** `age > 2` on a 32-bit value does not fit a gateway, so
   the three accepted values are constant table entries. Synapse emits gateway conditions and
   would hit the 4-byte limit.

7. **Deparser checksums, over fields split to suit them.** A `Checksum()` can neither read nor
   write a slice, so every field one touches has to be a header field in its own right: the IPv4
   protocol and header checksum are separate fields here, as are the TCP window, checksum and
   urgent pointer, rather than packed into the wider fields synapse's header guessing would
   produce. Synapse emits no dataplane checksum at all today.

Items 4 through 7 are ordinary emission rules synapse could adopt. Item 3 is a modest new module.
Items 1 and 2 are the real question, and 2 should be tested before 1 is attempted.

## What compiling cannot tell you

An earlier version of this program compiled in 4 s with 0 errors and was wrong in six ways. Five
are constraints the emitter has to respect; they are listed here because a synthesizer that only
checks "does bf-p4c accept it" will violate every one of them and never find out. The sixth was a
plain transcription slip on my part, hashing `ack` where the cookie check wants `ack - 1`, which
is the other reason `tests/smartcookie.py` exists.

1. **Statements inside an action run in order.** A two-field swap written as `a = b; b = a;`
   duplicates `b`. The old value has to be captured by an earlier action, because a temporary
   written and read inside one action makes it span stages, which bf-p4c does reject. A *single*
   whole-field write of a field in terms of itself is fine and is one operation:
   `ports = ports[15:0] ++ ports[31:16]` swaps correctly.
2. **The same ordering across tables.** `ack = seq + 1` has to read `seq` before the table that
   writes the cookie into it, so it is staged on the way into the pipeline.
3. **A deparser `Checksum()` can neither read nor write a slice.** The write is silently ignored
   and the packet keeps the checksum it arrived with, which is why item 7 above splits header
   fields instead of staging them into metadata: staging fixes the read side and does nothing for
   the write side.
4. **A recirculated packet still has to parse.** Marking a packet in flight by rewriting its
   ethertype made the ingress parser reject it on the way back round, so the second and third laps
   hashed nothing. The recirculation header's `code_path` is the marker; the ethertype is left
   alone.
5. **`f = C ++ (f[7:0] | K)` silently loses the OR.** This one is a bf-p4c bug, not a rule to
   follow: it allocates a temporary for the concat operand and never writes it
   (`set hdr.f.0-7, $concat_to_slice27`, with nothing anywhere assigning `$concat_to_slice27`), so
   the field comes out zero. Two slice assignments compile to `set hdr.f.8-15, C` plus
   `or B7, K, B7` and are correct. Reproducers: `concatE.p4` (broken), `concatF.p4` (correct).

None of these five appear in any solution synapse ships today: a scan of `synthesized/*.p4` finds
no in-action swap, no concat containing an operation, no deparser checksum and no ethertype
rewrite. So nothing is broken right now, but items 1 and 2 become live the moment the emitter is
taught to rewrite packets in place, and item 3 changes what header guessing has to produce before
a checksum can be emitted at all.
