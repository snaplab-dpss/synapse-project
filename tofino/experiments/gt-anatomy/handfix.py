#!/usr/bin/env python3
"""Re-apply the hand fixes synapse does not yet emit, so a synthesized SmartCookie can be tested.

usage: handfix.py <in.p4> <out.p4>

What used to be here. This file carried six fixes, each a synapse bug found by running
tests/smartcookie.py on the model. Five of them are now in synapse and have been removed:

  code_path carried EP node ids in a 16-bit field   -> 0a81ea541
  cpu header fields declared narrower than their    -> 7e7a46634
    BDD symbol, so P4 and C disagreed on the layout
  egress_state did not survive a recirculation      -> 64521180e
  the controller registered backing tables the P4   -> 6cef163fc
    never declares, and registered them repeatedly
  a zero bloom cleanup interval spun on the lock    -> a5c16d4d1 (in libsycon)

A sixth, unrelated to this file, fixed the whole hash chain evaluating to zero: 9b081a7b6.

What is left is the deparser checksums, which synapse still drops entirely
(`Ignore.cpp` lists `nf_set_rte_ipv4_udptcp_checksum` among the calls it ignores, and
`ModifyHeader.cpp` strips the checksum field write). That is work item 4 in
`tofino/exp-compute/GROUND-TRUTH.md`, and it is not SmartCookie-specific: it is a live
correctness bug in shipped solutions, which is why the entry there says nat fails
`tests/nat.py` on the fast path until `NAT_CHECK_CHECKSUMS=0`.

The field names below are anchors into synapse's guessed header layout, which moves. If an
assertion fires, re-read the emitted `hdr1_h`/`hdr2_h` and re-anchor rather than forcing it.
"""
import sys


def sub1(s, a, b, tag):
    assert s.count(a) == 1, "%s: %d occurrences" % (tag, s.count(a))
    return s.replace(a, b)


def fix_p4(s):
    # IPv4 bytes 8..11 are ttl, protocol, checksum: split so the checksum is its own field. A
    # Checksum() input that is a slice is a hard error, and an output written to a slice compiles
    # and is silently ignored, so every field a checksum touches has to be a field in its own right.
    h = s.index("header hdr1_h {")
    e = s.index("}", h)
    head, body, tail = s[:h], s[h:e], s[e:]
    assert body.count("  bit<32> data2;") == 1, "ipv4 layout"
    body = body.replace("  bit<32> data2;", "  bit<8> data2;\n  bit<8> data2b;\n  bit<16> data2c;")
    s = head + body + tail

    n = s.count("hdr.hdr1.data2[23:0][23:16]")
    assert n >= 1, "protocol reads"
    s = s.replace("hdr.hdr1.data2[23:0][23:16]", "hdr.hdr1.data2b")

    s = sub1(s, "  bit<16> data4;\n  bit<32> data5;\n}",
             "  bit<16> data4;\n  bit<16> data5;\n  bit<16> data5b;\n}", "tcp layout")
    s = sub1(s, "  bit<1> to_egress;", "  bit<1> to_egress;\n  bit<1> redo_checksum;\n  bit<16> tcp_len;", "flags")
    s = sub1(s, "    meta.dev = 0;", "    meta.dev = 0;\n    meta.redo_checksum = 0;", "flag init")

    anchor = "        hdr.hdr1.data0 = 8w0x45 ++ hdr.hdr1.data0[23:16] ++ 8w0x00 ++ 8w0x28;"
    s = sub1(s, anchor, anchor + "\n        meta.redo_checksum = 1;\n        meta.tcp_len = 20;", "flag set")

    s = sub1(s, """  in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {

  apply {
    pkt.emit(hdr);
  }
}""", """  in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
  Checksum() ipv4_checksum;
  Checksum() tcp_checksum;

  apply {
    if (meta.redo_checksum == 1) {
      hdr.hdr1.data2c = ipv4_checksum.update({
        hdr.hdr1.data0, hdr.hdr1.data1, hdr.hdr1.data2, hdr.hdr1.data2b, hdr.hdr1.data3, hdr.hdr1.data4
      });
      // bf-p4c rejects a literal in a checksum list, so the TCP length comes from metadata.
      hdr.hdr2.data5 = tcp_checksum.update({
        hdr.hdr1.data3, hdr.hdr1.data4, 8w0, hdr.hdr1.data2b, meta.tcp_len,
        hdr.hdr2.data0, hdr.hdr2.data1, hdr.hdr2.data2, hdr.hdr2.data3, hdr.hdr2.data4, hdr.hdr2.data5b
      });
    }
    pkt.emit(hdr);
  }
}""", "deparser")
    print("  checksums: added, gated on redo_checksum")
    return s


if __name__ == "__main__":
    ip4, op4 = sys.argv[1:3]
    print("P4:")
    open(op4, "w").write(fix_p4(open(ip4).read()))
    print("wrote %s" % op4)
