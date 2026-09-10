#!/usr/bin/env python3
"""Move the ingress->egress crossing earlier, to the boundary where fewest values are live.

usage: rebalance.py <in.p4> <out.p4> [cut]

Why. `tofino/exp-compute/GROUND-TRUTH.md` ends on this: "The plan should prefer pass boundaries
where few values are live, and nothing in the score expresses that today." This applies that by
hand, to check the claim before the cost term is built into the search.

Synapse cuts the pass after the whole ingress hash block (31 ops), where 10 values are live and the
state header carries 13. Measured over every candidate boundary in that block, cutting after op 2
leaves only `meta_s24_0` to cross: the other live values are packet fields the egress parser
extracts for itself, and the clock, which the egress reads from `global_tstamp`.

The result should look like the unrolled ground truth, which puts twice as much in egress as in
ingress:

                       ours (before)   unrolled GT
    ingress actions          100            79
    egress actions            34            54
    ingress 32-bit metadata   49             9

Also drops five fields the crossing writes and nothing ever reads -- `rotate_left_108_x_out` is
reused for the one value that still crosses, the other four go. Each is a `deparsed
exact_containers` 32-bit field, so each costs a container for the life of the program.
"""
import re
import sys

# The ingress hash block, by the first and last action it invokes.
BLOCK_FIRST = "compute_rotate_left_107"
BLOCK_LAST = "compute_rotate_left_132_h1"

# egress_state fields the crossing writes that nothing reads. The first four were already dead in
# synapse's output; the rest become dead once the moved ops compute them in the egress instead.
DEAD_CROSSED = ["op_shl_380_a_out", "op_or_381_b_out", "rotate_left_120_x_out", "rotate_left_132_x_out",
                "op_or_381_out", "rotate_left_131_out", "rotate_left_132_out", "rotate_left_133_out",
                "rotate_left_133_x_out", "op_add_386_out", "op_xor_387_out"]

# The egress body reads the crossed values only through these parser-staged aliases, so redirecting
# them to locally computed values is the whole rewiring. Right-hand sides are the expressions the
# old crossing used, with meta. -> eg_md.
# Bare control-locals the moved actions read. These are neither metadata nor header fields -- this
# one is the delta a vector-table lookup returns -- so the meta. -> eg_md. rename leaves them
# dangling and they have to cross like any other live value. bf-p4c names them precisely
# ("declaration not found"), which is how this list was built.
CROSS_LOCALS = {"vector_table_1073939504_105_get_value_param0": "vt_delta"}

STG_LOCAL = [
    ("stg_es386", "eg_md.meta_sn_10 + eg_md.meta_sn_12", False),
    ("stg_es381", "eg_md.meta_sn_19", False),
    ("stg_es387", "eg_md.meta_sn_18 ^ eg_md.meta_sn_15", False),
    ("stg_es131", "eg_md.meta_sn_18", False),
    ("stg_es132", "eg_md.meta_sn_12[23:0] ++ eg_md.meta_sn_12[31:24]", True),
    ("stg_es133", "eg_md.meta_sn_15[15:0] ++ eg_md.meta_sn_15[31:16]", True),
    ("stg_es133x", "eg_md.meta_sn_15", False),
]


def sub1(s, a, b, tag):
    assert s.count(a) == 1, "%s: %d occurrences" % (tag, s.count(a))
    return s.replace(a, b)


def main(src, dst, cut):
    s = open(src).read()
    lines = s.split("\n")
    bodies = {m.group(1): m.group(2) for m in re.finditer(r"\n  action (\w+)\(\) \{(.*?)\n  \}", s, re.S)}

    # --- the block's invocation sequence, located by its first and last action ------------------
    inv = [(i, re.match(r"(compute_\w+)\(\);", l.strip()).group(1))
           for i, l in enumerate(lines) if re.match(r"compute_\w+\(\);", l.strip())]
    starts = [k for k, (_, nm) in enumerate(inv) if nm == BLOCK_FIRST]
    ends = [k for k, (_, nm) in enumerate(inv) if nm == BLOCK_LAST]
    assert len(starts) == 1 and len(ends) == 1, "block anchors are not unique"
    block = inv[starts[0]:ends[0] + 1]
    moved = [nm for _, nm in block[cut:]]
    print("block of %d ops; keeping %d in ingress, moving %d to egress" % (len(block), cut, len(moved)))

    # --- every metadata name the moved actions touch, so the egress can declare them ------------
    names = set()
    for nm in moved:
        names |= set(re.findall(r"meta\.(\w+)", bodies[nm]))
    reads, writes = set(), set()
    for nm in moved:
        for st in bodies[nm].split("\n"):
            m = re.match(r"\s*(?:@in_hash \{ )?meta\.(\w+)(?:\[[^\]]*\])? = (.+?);", st)
            if m:
                writes.add(m.group(1))
                reads |= set(re.findall(r"meta\.(\w+)", m.group(2)))
    live_in = reads - writes
    # Packet fields the egress parser extracts for itself, and the clock it reads itself.
    self_served = {"stg_h1d3": "hdr.hdr1.data3", "stg_h1d4": "hdr.hdr1.data4",
                   "stg_h2d0": "hdr.hdr2.data0", "time": None}
    must_cross = sorted(live_in - set(self_served))
    print("crossing payload: %s   (self-served in egress: %s)"
          % (must_cross, sorted(live_in & set(self_served))))
    assert must_cross == ["meta_s24_0"], must_cross

    # --- ingress: drop the moved invocations ----------------------------------------------------
    drop_lines = {i for i, nm in block[cut:]}
    lines = [l for i, l in enumerate(lines) if i not in drop_lines]
    s = "\n".join(lines)

    # --- ingress: move the action definitions out ----------------------------------------------
    moved_defs = []
    for nm in moved:
        m = re.search(r"\n  action %s\(\) \{.*?\n  \}" % re.escape(nm), s, re.S)
        assert m, nm
        body = m.group(0)
        s = s[:m.start()] + s[m.end():]
        body = re.sub(r"\bmeta\.", "eg_md.", body)
        for local, fld in CROSS_LOCALS.items():
            body = re.sub(r"\b%s\b" % re.escape(local), "eg_md.%s" % fld, body)
        moved_defs.append(body)

    # --- the crossing now carries one value -----------------------------------------------------
    old_cross = re.search(r"(\n +)@in_hash \{ hdr\.egress_state\.rotate_left_108_x_out.*?"
                          r"@in_hash \{ hdr\.egress_state\.op_xor_387_out[^\n]*\n", s, re.S)
    assert old_cross, "crossing block not found"
    ind = old_cross.group(1)
    s = (s[:old_cross.start()]
         + ind + "// The pass is cut where only this one value is live; everything after it runs in"
         + ind + "// the egress, which extracts the packet fields it needs and reads its own clock."
         + ind + "@in_hash { hdr.egress_state.rotate_left_108_x_out = meta.meta_s24_0; }"
         + "".join(ind + "@in_hash { hdr.egress_state.%s = %s; }" % (f, l) for l, f in CROSS_LOCALS.items())
         + "\n"
         + s[old_cross.end():])

    # --- egress_state gains a slot for each crossing local --------------------------------------
    s = sub1(s, "header egress_state_h {\n  bit<32> dev;",
             "header egress_state_h {\n  bit<32> dev;"
             + "".join("\n  bit<32> %s;" % f for f in CROSS_LOCALS.values()), "egress_state decl")

    # --- egress_state loses the fields nothing reads --------------------------------------------
    for f in DEAD_CROSSED:
        s = sub1(s, "  bit<32> %s;\n" % f, "", "egress_state.%s" % f)

    # --- the moved ops reuse the egress's own slot pool ------------------------------------------
    # Both pools come from the same liveness pooling and are named by the same cut signature, so
    # they line up by name. The two are strictly sequential -- the moved ops finish before the
    # egress's own body starts, handing over through the stg_es* aliases -- so one pool serves both
    # and the egress does not pay twice for the same chain.
    native = sorted(set(re.findall(r"eg_md_sn_(\d+)", s)), key=int)
    imported = sorted(set(re.findall(r"\bmeta_sn_(\d+)\b", "".join(moved_defs))), key=int)
    remap = {}
    for k, src_i in enumerate(imported):
        if k < len(native):
            remap["meta_sn_" + src_i] = "eg_md_sn_" + native[k]
    for sig in re.findall(r"\bmeta_(s\d+_\d+)\b", "".join(moved_defs)):
        if ("eg_md_" + sig) in s:
            remap["meta_" + sig] = "eg_md_" + sig
    def apply_remap(txt):
        return re.sub(r"\bmeta_(?:sn_\d+|s\d+_\d+)\b", lambda m: remap.get(m.group(0), m.group(0)), txt)
    moved_defs = [apply_remap(d) for d in moved_defs]
    names = {remap.get(n, n) for n in names}
    print("merged %d imported slots onto the egress pool" % len(remap))

    # --- egress metadata declares what the moved actions use ------------------------------------
    h = s.index("struct synapse_egress_metadata_t {")
    e = s.index("\n}", h)
    have = set(re.findall(r"bit<32> (\w+);", s[h:e]))
    names |= set(CROSS_LOCALS.values())
    decls = "".join("\n  bit<32> %s;" % n for n in sorted(names) if n not in have)
    s = s[:e] + "\n" + decls + s[e:]

    # --- egress parser: stage the crossed value and the packet fields; drop the old stagings ----
    stage = apply_remap("\n    eg_md.meta_s24_0 = hdr.egress_state.rotate_left_108_x_out;")
    for _, fld in CROSS_LOCALS.items():
        stage += "\n    eg_md.%s = hdr.egress_state.%s;" % (fld, fld)
    for n, src_fld in sorted(self_served.items()):
        if n in live_in and src_fld:
            stage += "\n    eg_md.%s = %s;" % (n, src_fld)
    # scoped to the egress parser: the ingress parser also extracts hdr2
    ep = s.index("parser EgressParser(")
    ep_end = s.index("\ncontrol Egress(", ep)
    head, pblk, tail = s[:ep], s[ep:ep_end], s[ep_end:]
    pblk = sub1(pblk, "    pkt.extract(hdr.hdr2);", "    pkt.extract(hdr.hdr2);" + stage, "egress parser stage")
    for alias, _, _ in STG_LOCAL:
        pblk = re.sub(r"\n    eg_md\.%s = hdr\.egress_state\.\w+;" % alias, "", pblk)
    s = head + pblk + tail

    # --- egress: run the moved ops first, then feed the aliases the egress body already reads ---
    # the action definitions go inside control Egress, ahead of its apply block
    egc = s.index("control Egress(")
    apply_at = s.index("\n  apply {", egc)
    s = s[:apply_at] + "\n" + "".join(moved_defs) + s[apply_at:]

    calls = "".join("\n    %s();" % nm for nm in moved)
    for alias, rhs, hashed in STG_LOCAL:
        rhs = apply_remap(rhs)
        stmt = "@in_hash { eg_md.%s = %s; }" % (alias, rhs) if hashed else "eg_md.%s = %s;" % (alias, rhs)
        calls += "\n    " + stmt
    # `op_lshr_449_out` is produced here now, and the next ingress lap still reads it.
    calls += apply_remap("\n    @in_hash { hdr.egress_state.op_lshr_449_out = eg_md.meta_sn_14 >> 32w0x0000000c; }")
    s = sub1(s, "    eg_md.time = eg_intr_md_from_prsr.global_tstamp[47:16];",
             "    eg_md.time = eg_intr_md_from_prsr.global_tstamp[47:16];" + calls, "egress apply head")

    # the moved actions took their locals with them; drop the ingress declarations nothing uses
    ig, eg = s.index("control Ingress("), s.index("control Egress(")
    ing_used = set(re.findall(r"meta\.(\w+)", s[:eg]))
    h = s.index("struct synapse_ingress_metadata_t {")
    e = s.index("\n}", h)
    kept, dropped = [], 0
    for line in s[h:e].split("\n"):
        m = re.match(r"  bit<\d+> (\w+);", line)
        if m and m.group(1) not in ing_used:
            dropped += 1
            continue
        kept.append(line)
    s = s[:h] + "\n".join(kept) + s[e:]
    print("pruned %d unused ingress metadata fields" % dropped)

    open(dst, "w").write(s)
    print("wrote %s" % dst)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], int(sys.argv[3]) if len(sys.argv) > 3 else 2)
