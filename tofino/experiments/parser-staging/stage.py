#!/usr/bin/env python3
"""Stage deparsed header operands into metadata in the parser, so chain arithmetic never
touches a `deparsed exact_containers` field directly.

usage: stage.py <in.p4> <out.p4>

Why. A deparsed header field carries the deparser's byte grid {8,16,24} and cannot be split.
`a = b ^ hdr.X` forces a, b and hdr.X into a shared container layout, so that grid unifies with
the chain's own rotate cuts {16,19,24,25,27} and every value in the chain ends up needing seven
slices. Seven slices is seven PHV sources when the allocator cannot co-locate them, against a
limit of two -- which is the "too many sources" error.

A *copy* does not unify: it needs one source, not an aligned pair. So copying the header field
into metadata once, in the parser, and operating on the copy costs no MAU stage and no hash unit,
and the copy carries only the rotate cuts. toy.p4 in this directory verifies both halves: the
idiom compiles, and the staged copy lands in one container sliced only at its own rotate cut.

This is what the ground truth does with its message word (`action msg_src() { meta.msg =
hdr.hdr1.data5; }` then `v3 = v3 ^ meta.msg`) -- it never mixes a packet field into the chain.
"""
import re
import sys

# field -> alias, per gress. Only 32-bit fields that appear as an operand of chain arithmetic;
# 8-bit uses (hdr.hdr2.data3[7:0]) do not carry the 32-bit grid and are left alone.
INGRESS = {
    "hdr.hdr1.data3": "meta.stg_h1d3",
    "hdr.hdr1.data4": "meta.stg_h1d4",
    "hdr.hdr2.data0": "meta.stg_h2d0",
    "hdr.hdr2.data1": "meta.stg_h2d1",
    "hdr.hdr2.data2": "meta.stg_h2d2",
    "hdr.recirc.f32_1": "meta.stg_rc1",
    "hdr.recirc.f32_2": "meta.stg_rc2",
    "hdr.recirc.f32_3": "meta.stg_rc3",
    "hdr.recirc.f32_4": "meta.stg_rc4",
    "hdr.recirc.f32_6": "meta.stg_rc6",
    "hdr.recirc.f32_7": "meta.stg_rc7",
    "hdr.egress_state.op_lshr_449_out": "meta.stg_es449",
}

EGRESS = {
    "hdr.hdr2.data1": "eg_md.stg_h2d1",
    "hdr.egress_state.op_add_386_out": "eg_md.stg_es386",
    "hdr.egress_state.op_or_381_out": "eg_md.stg_es381",
    "hdr.egress_state.op_xor_387_out": "eg_md.stg_es387",
    "hdr.egress_state.rotate_left_131_out": "eg_md.stg_es131",
    "hdr.egress_state.rotate_left_132_out": "eg_md.stg_es132",
    "hdr.egress_state.rotate_left_133_out": "eg_md.stg_es133",
    "hdr.egress_state.rotate_left_133_x_out": "eg_md.stg_es133x",
}

# Where each staged copy is written: the parser state that has just extracted the header.
INGRESS_SITES = [
    ("  state parse_recirc {\n    pkt.extract(hdr.recirc);\n    pkt.extract(hdr.egress_state);",
     ["hdr.recirc.f32_1", "hdr.recirc.f32_2", "hdr.recirc.f32_3", "hdr.recirc.f32_4",
      "hdr.recirc.f32_6", "hdr.recirc.f32_7", "hdr.egress_state.op_lshr_449_out"]),
    ("  state parser_4 {\n    pkt.extract(hdr.hdr1);",
     ["hdr.hdr1.data3", "hdr.hdr1.data4"]),
    ("  state parser_11 {\n    pkt.extract(hdr.hdr2);",
     ["hdr.hdr2.data0", "hdr.hdr2.data1", "hdr.hdr2.data2"]),
]


def sub1(s, a, b, tag):
    assert s.count(a) == 1, "%s: %d occurrences" % (tag, s.count(a))
    return s.replace(a, b)


def declare(s, struct, aliases):
    """Add the alias fields to a metadata struct."""
    h = s.index("struct %s {" % struct)
    e = s.index("\n}", h)
    decls = "".join("\n  bit<32> %s;" % a.split(".", 1)[1] for a in sorted(set(aliases.values())))
    return s[:e] + "\n" + decls + s[e:]


def rewrite_reads(block, aliases):
    """Replace header reads on the RHS of metadata assignments -- and only there.

    Writes (the field on the left), the deparser's checksum lists, `pkt.emit`, the swap actions
    and the parser's own staging copies must all keep naming the header.
    """
    out, n = [], 0
    assign = re.compile(r"^(\s*(?:meta|eg_md)\.[A-Za-z0-9_]+(?:\[\d+:\d+\])? = )(.+;)\s*$")
    for line in block.split("\n"):
        m = assign.match(line)
        if m and "pkt.extract" not in line:
            lhs, rhs = m.group(1), m.group(2)
            new = rhs
            for fld, alias in aliases.items():
                new = re.sub(r"\bhdr\." + re.escape(fld.split(".", 1)[1]) + r"\b",
                             alias, new) if new.count(fld) else new
            if new != rhs:
                # a bare copy already costs one source; leave it on the header
                if not re.fullmatch(r"\(?(?:meta|eg_md)\.[A-Za-z0-9_]+\)?;", new):
                    n += 1
                    line = lhs + new
        out.append(line)
    return "\n".join(out), n


def main(src, dst):
    s = open(src).read()

    s = declare(s, "synapse_ingress_metadata_t", INGRESS)
    s = declare(s, "synapse_egress_metadata_t", EGRESS)

    for anchor, fields in INGRESS_SITES:
        copies = "".join("\n    %s = %s;" % (INGRESS[f], f) for f in fields)
        s = sub1(s, anchor, anchor + copies, "ingress staging @ %r" % anchor.split("\n")[0])
    print("ingress: %d staged copies in the parser" % sum(len(f) for _, f in INGRESS_SITES))

    anchor = "    pkt.extract(hdr.hdr2);\n    transition accept;"
    copies = "".join("\n    %s = %s;" % (a, f) for f, a in sorted(EGRESS.items()))
    s = sub1(s, anchor, "    pkt.extract(hdr.hdr2);" + copies + "\n    transition accept;",
             "egress staging")
    print("egress:  %d staged copies in the parser" % len(EGRESS))

    ig, eg = s.index("control Ingress("), s.index("control Egress(")
    head, ibody, ebody = s[:ig], s[ig:eg], s[eg:]
    ibody, ni = rewrite_reads(ibody, INGRESS)
    ebody, ne = rewrite_reads(ebody, EGRESS)
    print("rewrote %d ingress and %d egress arithmetic sites onto the staged copies" % (ni, ne))

    open(dst, "w").write(head + ibody + ebody)
    print("wrote %s" % dst)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
