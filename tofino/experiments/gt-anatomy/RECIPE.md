# The compiling SmartCookie build

    ./run.sh <tag> 1 2 3 4 H A2 D G W

`base.p4` is synapse's output, unmodified. `compiling.p4` is the exact result of that command,
kept so the artifact survives even if `fixes.py` drifts; regenerate it rather than editing it.
`compiling.cpp` is the controller, unchanged from synapse's output.

Verified: 0 errors, bf-rt.json 647 KB, 17 ingress / 18 egress stages, 108 tables, egress live
(241 egress field slices in PHV; a dead egress shows 26). See README.md Part 4.

## Controller fix (CT), applied to compiling.cpp by hand

Synapse's controller constructs
`VectorTable(..., {"Ingress.vector_table_1073939504_105", "…_88" x4})`, but the P4 declares only
`_105`: node 88's read was offloaded to the controller, so its dataplane table is never emitted,
while the controller still registers it. libsycon then aborts in `build_table` ("Object not
found"). The four duplicates would also be wrong on their own -- `VectorTable` sums `value_size`
across the list, so replicas must not repeat.

This is a **pre-existing synapse bug**, not a consequence of the P4 fixes: synapse's unmodified
`base.p4` has the same single `_105` table.

## Model test (tests/smartcookie.py on the Tofino 2 model)

    ./run.sh <tag> 1 2 3 4 H A2 D G W J CS   # J and CS are correctness fixes found by the test

Then, in the SDE container as root, bring the testbed up and run the test as two separate steps --
`--up` immediately followed by the test races the controller's table population and the first
packet misses `ingress_port_to_nf_dev`:

    python3 ./testbed.py up <tag> ; sleep 20
    SC_NF=<tag> SC_SERVER_PORT=1 SC_SERVER_DEV=0 python3 ./smartcookie.py

Ground truth `smartcookie-manual` PASSES on the same harness, so the testbed and topology are good.

### What the test found

**fJ -- parser protocol dispatch inverted (synapse bug, in the unmodified output).**
IPv4 protocol 0x11 (UDP) reached `pkt.extract(hdr.hdr2)` (160 bits, the TCP header) and 0x06 (TCP)
fell through to `pkt.extract(hdr.hdr3)` (64 bits, UDP); the nested state also re-tested the same
field for a value it could not hold. A TCP packet never got a TCP header, so the entire cookie path
was unreachable and a client SYN was simply routed to the server. Fixed by fJ.

**CT -- controller references a table the P4 does not declare (synapse bug).** See above.

**Remaining: synapse emits no deparser checksums.** With fJ the SYN-ACK is produced correctly --
addresses and ports swapped, `flags = SA`, `ack = seq + 1`, and the test's expected packet is built
from the cookie our switch minted, so the cookie path is working. The two packets are byte-identical
**except the TCP checksum** (expected `523d`, got `524e`). Our `IngressDeparser` is just
`pkt.emit(hdr)`; the ground truth recomputes both IPv4 and TCP checksums in its egress deparser.

Fixing this by hand needs the checksum fields to be addressable: ours sit inside wider guessed
fields (IPv4 checksum is `hdr1.data2[15:0]`, TCP checksum is `hdr2.data4[31:16]`), whereas the
ground truth's layout gives each its own field. So it is a header re-cut plus two `Checksum()`
externs, in the same family as fix 1.

### fCS -- deparser checksums (synapse gap), and the first test now passes

Synapse emits no deparser checksums. `fCS` splits the two checksum fields out of the wider guessed
fields (IPv4 bytes 9..11 protocol+checksum -> 8 + 16; TCP bytes 14..19 window+checksum+urgent ->
16 + 16 + 16) and adds two `Checksum()` externs, guarded by a `redo_checksum` flag set by the one
block that rewrites the packet -- recomputing unconditionally would corrupt pass-through packets,
whose payload the deparser cannot see. bf-p4c also rejects a literal in a checksum list
("Non-zero constant entry in checksum calculation not implemented yet"), so the TCP length is
staged through `meta.tcp_len`, exactly as the ground truth carries `eg_md.tcp_len`.

With that, **the first test case passes**:

    [*] a client SYN is answered with a SYN-ACK carrying a cookie
      send  5: tcp 246.69.219.112:61478 -> 0.1.1.1:9763
      recv  5: tcp 0.1.1.1:9763 -> 246.69.219.112:61478

The test builds its expected packet from the cookie the switch minted and compares with checksums
included, so this exercises the SipHash chain, the crossing, the recirculation and both deparser
checksums end to end.

### Where it stops now: the controller, not the P4

Case 2 ("the client's ACK with a valid cookie reaches the server, ECE-tagged and with seq - 1")
gets no packet. The model trace shows the ACK doing the right thing in the dataplane -- bloom miss,
so the cookie-verification path, SipHash started -- and then `tbl_build_cpu_hdr` sending it to
`ucast_egress_port = 0`, the controller. That is BDD node 88, which the search offloaded. The
controller logs one `RX` and emits nothing.

So the remaining work is in synapse's generated controller (the offloaded node's replay), not in
the P4. Note the P4 has no ECE/seq-1 write anywhere, consistent with that whole branch living in
the controller.
