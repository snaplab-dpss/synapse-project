#!/usr/bin/env python3
"""Apply named hand fixes to synapse's SmartCookie output, to test one hypothesis at a time.

usage: fixes.py <in.p4> <out.p4> <fix> [fix ...]

Fixes 1-4 were each validated separately: every one removed a specific named bf-p4c error.
Fixes A-C come from diffing against the hand-written ground truth (smartcookie-unrolled.p4),
which compiles in 9.5 s; each names something the ground truth does differently.
"""
import re, sys


def sub1(s, a, b, tag):
    assert s.count(a) == 1, "%s: expected 1 occurrence, found %d" % (tag, s.count(a))
    return s.replace(a, b)


def f1_header_recut(s):
    """The 32-bit TCP ack straddles the guessed 24/24 boundary, so it is read as a concat and
    cannot be an ALU operand. Re-cut so a whole-value read is a whole field."""
    s = sub1(s, "  bit<24> data2;\n  bit<24> data3;\n", "  bit<32> data2;\n  bit<16> data3;\n", "f1 layout")
    s = sub1(s, "meta.op_add_336_out = 32w0xffffffff + (hdr.hdr2.data2 ++ hdr.hdr2.data3[23:16]);",
             "meta.op_add_336_out = 32w0xffffffff + hdr.hdr2.data2;", "f1 read")
    s = sub1(s, "hdr.hdr2.data2 = meta.hdr_val1[31:24] ++ meta.hdr_val1[23:16] ++ meta.hdr_val1[15:8];",
             "hdr.hdr2.data2 = meta.hdr_val1;", "f1 write2")
    s = sub1(s, "hdr.hdr2.data3 = meta.hdr_val1[7:0] ++ 8w0x50 ++ meta.hdr_val2[7:0];",
             "hdr.hdr2.data3 = 8w0x50 ++ meta.hdr_val2[7:0];", "f1 write3")
    return s


def f2_identity_concat(s):
    """x[31:24]++x[23:16]++x[15:8]++x[7:0] is the identity, but bf-p4c reads it as 4 sources."""
    pat = re.compile(r"([A-Za-z0-9_.]+)\[31:24\] \+\+ \1\[23:16\] \+\+ \1\[15:8\] \+\+ \1\[7:0\]")
    s, n = pat.subn(lambda m: m.group(1), s)
    assert n >= 1, "f2: no identity concat found"
    return s


def f3_natural_width(s):
    """An 8-bit TCP-flags OR widened to 32 bits needs a cast that violates action constraints."""
    s = sub1(s, "  bit<32> hdr_val2;", "  bit<8> hdr_val2;", "f3 decl")
    s = sub1(s, "meta.hdr_val2 = ((bit<32>)(hdr.hdr2.data3[7:0])) | (32w0x00000012);",
             "meta.hdr_val2 = (hdr.hdr2.data3[7:0]) | (8w0x12);", "f3 op")
    s = sub1(s, "hdr.hdr2.data3 = 8w0x50 ++ meta.hdr_val2[7:0];",
             "hdr.hdr2.data3 = 8w0x50 ++ meta.hdr_val2;", "f3 use")
    return s


def f4_in_hash_rotates(s):
    """A byte-aligned concat rotate is legal bare but still cuts its operand's container."""
    lines = s.split("\n")
    pat = re.compile(r"^(\s*)([A-Za-z0-9_.]+) = ([A-Za-z0-9_.]+)\[(\d+):0\] \+\+ \3\[31:(\d+)\];\s*$")
    n = 0
    for i, line in enumerate(lines):
        if "@in_hash" in line:
            continue
        m = pat.match(line)
        if not m or int(m.group(4)) + 1 != int(m.group(5)):
            continue
        lines[i] = "%s@in_hash { %s = %s[%s:0] ++ %s[31:%s]; }" % (
            m.group(1), m.group(2), m.group(3), m.group(4), m.group(3), m.group(5))
        n += 1
    assert n > 0, "f4: no bare rotates found"
    print("  f4: wrapped %d bare rotates" % n)
    return "\n".join(lines)


def fA_cpu_header(s):
    """The ground truth's cpu header has 3 fields and no computed values; ours carries 12, each a
    deparsed exact_containers field sharing a supercluster with the metadata it copies."""
    hdr = re.search(r"(header cpu_h \{\n)(.*?)(\n\})", s, re.S)
    body = hdr.group(2)
    keep, dropped = [], []
    for line in body.split("\n"):
        m = re.match(r"\s*bit<\d+> (\w+);", line)
        if m and re.match(r"(rotate_left|op_|bf_)", m.group(1)):
            dropped.append(m.group(1))
        else:
            keep.append(line)
    assert dropped, "fA: no computed fields in cpu_h"
    s = s[:hdr.start(2)] + "\n".join(keep) + s[hdr.end(2):]
    for name in dropped:
        s = re.sub(r"^\s*hdr\.cpu\.%s = [^;]+;\n" % re.escape(name), "", s, flags=re.M)
    print("  fA: dropped %d computed fields from cpu_h" % len(dropped))
    return s


def fB_whole_field_key(s):
    """The ground truth hashes whole header fields; we slice each address into 24+8, which makes
    the address a sub-word read as well as a whole ALU operand, and it fragments."""
    s = sub1(s, "  bit<24> key_24b_0;\n  bit<8> key_8b_1;\n  bit<24> key_24b_2;\n  bit<8> key_8b_3;\n",
             "  bit<32> key_32b_a;\n  bit<32> key_32b_b;\n", "fB decl")
    s = re.sub(r"\s*meta\.key_24b_0 = hdr\.hdr1\.data3\[31:8\];\n\s*meta\.key_8b_1 = hdr\.hdr1\.data3\[7:0\];"
               r"\n\s*meta\.key_24b_2 = hdr\.hdr1\.data4\[31:8\];\n\s*meta\.key_8b_3 = hdr\.hdr1\.data4\[7:0\];",
               "\n                meta.key_32b_a = hdr.hdr1.data3;\n                meta.key_32b_b = hdr.hdr1.data4;", s)
    s = re.sub(r"\s*meta\.key_24b_0,\n\s*meta\.key_8b_1,\n\s*meta\.key_24b_2,\n\s*meta\.key_8b_3,",
               "\n      meta.key_32b_a,\n      meta.key_32b_b,", s)
    assert "key_24b_0" not in s, "fB: leftover key_24b_0"
    return s


def fC_device_in_hash(s):
    """The ground truth takes the egress port from a table action parameter, never from a byte
    read of the address. @in_hash is the documented escape for such a sub-word read."""
    pat = "nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:8][23:16]);"
    n = s.count(pat)
    assert n >= 1, "fC: device read not found"
    s = s.replace(pat, "@in_hash { nf_dev[15:0] = (bit<16>)(hdr.hdr1.data4[31:24]); }")
    print("  fC: wrapped %d device byte reads" % n)
    return s


def fD_header_write_in_hash(s):
    """The ground truth writes a compute result into a deparsed header field through the hash unit
    (`@in_hash { hdr.hdr2.data2 = ctime ^ v0 ^ v1 ^ v2 ^ v3; }`), so the field is never an ALU
    destination fed by cut sources. Ours copies it plainly from cut metadata, which needs three
    PHV sources. Wrap the write; deliberately no folding of the producing expression, because
    these are bare statements and reordering would change which value of data1 is read."""
    n = 0
    for dst, src in [("hdr.hdr2.data1", "meta.hdr_val0"), ("hdr.hdr2.data2", "meta.hdr_val1")]:
        a = "        %s = %s;" % (dst, src)
        b = "        @in_hash { %s = %s; }" % (dst, src)
        assert s.count(a) == 1, "fD: %s" % dst
        s = s.replace(a, b)
        n += 1
    print("  fD: routed %d header writes through the hash unit" % n)
    return s


def fE_stage_header_read(s):
    """The ground truth never lets the chain read a packet header directly; it stages through
    meta.msg. Ours reads the deparsed ack straight into an ALU op."""
    a = "meta.op_add_336_out = 32w0xffffffff + hdr.hdr2.data2;"
    assert s.count(a) == 1, "fE: read site (needs fix 1 first)"
    s = s.replace(a, "meta.tcp_ack_staged = hdr.hdr2.data2;\n    meta.op_add_336_out = 32w0xffffffff + meta.tcp_ack_staged;")
    s = s.replace("  bit<32> hdr_val0;", "  bit<32> tcp_ack_staged;\n  bit<32> hdr_val0;", 1)
    return s


def fG_one_hash_per_action(s):
    """Fix 4 wraps every concat rotate in @in_hash, which puts two 32-bit hash results in one
    action for the rotates synapse had classified as ALU. bf-p4c allows 32 bits through a table's
    immediate pathway, so split them. The ground truth never needs this: it carries exactly one
    @in_hash per action. In synapse this is the existing MAX_HASH_BITS_PER_ACTION budget -- the
    real fix there is to classify byte-aligned concat rotates as Hash ops, not ALU ops."""
    import re as _re
    act = _re.compile(r"(  action (\w+)\(\) \{)(.*?)(\n  \})", _re.S)
    extra, edits = {}, []
    for m in act.finditer(s):
        head, name, body, tail = m.group(1), m.group(2), m.group(3), m.group(4)
        lines = body.split("\n")
        idx = [i for i, l in enumerate(lines) if "@in_hash" in l]
        if len(idx) <= 1:
            continue
        keep = [l for i, l in enumerate(lines) if i not in idx[1:]]
        moved = [lines[i].strip() for i in idx[1:]]
        names, defs = [], []
        for k, stmt in enumerate(moved):
            nm = "%s_h%d" % (name, k + 1)
            names.append(nm)
            defs.append("  action %s() {\n    %s\n  }\n" % (nm, stmt))
        extra[name] = names
        edits.append((m.start(), m.end(), head + "\n".join(keep) + tail + "\n" + "".join(defs).rstrip("\n")))
    for start, end, rep in reversed(edits):
        s = s[:start] + rep + s[end:]

    def add(mm):
        indent, nm = mm.group(1), mm.group(2)
        if nm not in extra:
            return mm.group(0)
        return mm.group(0) + "".join("\n%s%s();" % (indent, e) for e in extra[nm])

    s = _re.sub(r"^([ \t]+)(\w+)\(\);", add, s, flags=_re.M)
    print("  fG: split %d actions holding more than one @in_hash" % len(extra))
    return s


def fA2_cpu_write_in_hash(s):
    """Semantics-preserving alternative to fA. The problem is not that the cpu header carries
    computed values, it is that `hdr.cpu.X = meta.X` copies a cut metadata field into a deparsed
    exact_containers field, which needs one PHV source per slice. Route the copy through the hash
    unit, as the ground truth does for its own header writes, and the value still reaches the
    controller."""
    import re as _re
    n = 0

    def wrap(m):
        nonlocal n
        n += 1
        return "%s@in_hash { hdr.cpu.%s = %s; }" % (m.group(1), m.group(2), m.group(3))

    s = _re.sub(r"^([ \t]+)hdr\.cpu\.((?:rotate_left|op_|bf_)\w+) = ([^;]+);", wrap, s, flags=_re.M)
    assert n > 0, "fA2: no cpu computed-field writes"
    print("  fA2: routed %d cpu header writes through the hash unit" % n)
    return s


def fH_egress_parses_recirc(s):
    """CORRECTNESS. On the crossing path the ingress validates BOTH hdr.recirc (build_recirc_hdr)
    and hdr.egress_state, and recirc comes first in the header struct, so the deparser emits
    [recirc][egress_state][hdr0][hdr1][hdr2]. EgressParser skipped hdr.recirc, a 40-byte misparse,
    which left hdr.recirc invalid in egress; the egress's eight result writes to hdr.recirc.f32_*
    then went nowhere and bf-p4c eliminated the whole egress chain as dead. Egress only runs when
    bypass_egress==0, i.e. exactly on that path, so extracting both unconditionally is right."""
    a = "    tofino_parser.apply(pkt, eg_intr_md);\n    pkt.extract(hdr.egress_state);"
    b = "    tofino_parser.apply(pkt, eg_intr_md);\n    pkt.extract(hdr.recirc);\n    pkt.extract(hdr.egress_state);"
    assert s.count(a) == 1, "fH: egress parser start"
    return s.replace(a, b)


def fW_deparsed_writes_in_hash(s):
    """Generalises A2 and D. Every write of a compute value into a deparsed header field --
    hdr.cpu.*, hdr.egress_state.*, hdr.recirc.* -- copies a metadata field that the rotate chain
    has cut into a `deparsed exact_containers` field, which then needs one PHV source per slice.
    The ground truth never does this as a plain ALU copy; it writes such fields through the hash
    unit (`@in_hash { hdr.hdr2.data2 = ctime ^ v0 ^ v1 ^ v2 ^ v3; }`). These are bare statements
    in the apply block, so each is its own table and the one-@in_hash-per-table limit is safe."""
    import re as _re
    n = 0

    def wrap(m):
        nonlocal n
        n += 1
        return "%s@in_hash { hdr.%s.%s = %s; }" % (m.group(1), m.group(2), m.group(3), m.group(4))

    # Only values the rotate chain produces: those are the fields it cuts. meta.ingress_port and
    # meta.dev are not, and wrapping them put a second @in_hash into build_recirc_hdr, which takes
    # action data -- 48 bits through the immediate pathway, and hash needs the hit pathway.
    # Only copies whose SOURCE is actually cut somewhere need the hash unit. The chain's results
    # (xor/add outputs, rotate outputs) are written whole and copy fine with an ALU. Wrapping them
    # too costs stages: each @in_hash is its own table and only three fit per stage (6 hash-dist
    # units, 2 per 32-bit op), which is exactly what pushed the egress from 20 to 23 stages.
    cut_names = set(_re.findall(r"(?:meta|eg_md)\.(\w+)\[", s))

    def wrap_if_cut(m):
        if m.group(4).split(".", 1)[1] not in cut_names:
            return m.group(0)
        return wrap(m)

    s = _re.sub(r"^([ \t]+)hdr\.(cpu|egress_state|recirc)\.(\w+) = ((?:meta|eg_md)\.(?:rotate_left|op_|bf_)\w+);",
                wrap_if_cut, s, flags=_re.M)
    assert n > 0, "fW: no deparsed compute writes"
    print("  fW: routed %d deparsed header writes through the hash unit" % n)
    return s


def _pool_one_gress(s, control_name, struct_name, prefix):
    """Liveness slot pooling for one gress. The ground truth reuses 8 32-bit values every round;
    synapse allocates a fresh field per operation, so every cut a rotate makes reaches all of them
    and the allocator's search explodes. Slots are assigned by linear scan over the order in which
    the apply block calls the compute actions.

    Soundness: the apply block is loop-free (recirculation is a new pass, not a back edge), so two
    values whose *textual* live ranges are disjoint can never be simultaneously live, and values in
    exclusive branches are merged only when their ranges are disjoint anyway. Anything that must
    outlive the pass already lives in a header (hdr.egress_state / hdr.recirc), not here."""
    import re as _re

    mstruct = _re.search(r"(struct %s \{)(.*?)(\n\})" % struct_name, s, _re.S)
    poolable = [n for w, n in _re.findall(r"bit<(\d+)>\s+(\w+);", mstruct.group(2))
                if w == "32" and _re.match(r"(rotate_left|op_)", n)]
    if not poolable:
        return s, 0, 0
    pool = set(poolable)

    ctl_start = s.index("control %s(" % control_name)
    ctl_end = s.index("control %sDeparser(" % control_name)
    ctl = s[ctl_start:ctl_end]

    # A field touched anywhere outside this control (parser, deparser, other gress) is not ours.
    outside = s[:ctl_start] + s[ctl_end:]
    pool = {n for n in pool if not _re.search(r"%s\.%s\b" % (prefix, _re.escape(n)), outside)}

    bodies = {m.group(1): m.group(2) for m in _re.finditer(r"  action (\w+)\(\) \{(.*?)\n  \}", ctl, _re.S)}

    def defs_uses(text):
        d, u = set(), set()
        for st in text.split(";"):
            if "=" not in st:
                continue
            lhs, rhs = st.split("=", 1)
            d |= {n for n in _re.findall(r"%s\.(\w+)" % prefix, lhs) if n in pool}
            u |= {n for n in _re.findall(r"%s\.(\w+)" % prefix, rhs) if n in pool}
        return d, u

    apply_txt = ctl[ctl.index("  apply {"):]
    # The execution sequence: each action call contributes its body, each bare statement itself.
    seq = []
    for line in apply_txt.split("\n"):
        st = line.strip()
        if st.startswith("//"):
            continue
        m = _re.match(r"(\w+)\(\);", st)
        if m and m.group(1) in bodies:
            seq.append(defs_uses(bodies[m.group(1)]))
        elif prefix + "." in st:
            seq.append(defs_uses(st))

    first, last = {}, {}
    for i, (d, u) in enumerate(seq):
        for v in d | u:
            first.setdefault(v, i)
            last[v] = i
    # A value never seen in the apply order still needs a slot of its own.
    for v in pool:
        if v not in first:
            first[v], last[v] = -1, 1 << 30

    # A slot carries the union of the cuts of every value that ever lives in it, and one sub-word
    # boundary anywhere fragments the whole chain -- which is why plain liveness pooling made
    # things worse. The ground truth does not hit this because SipHash gives each word a fixed
    # pair of rotations (v1 only ever 27 and 19, a0 only 16). Reproduce that: a slot is shared
    # only between values with an identical set of cut boundaries.
    cuts = {}
    for m in _re.finditer(r"%s\.(\w+) = %s\.(\w+)\[(\d+):0\] \+\+ %s\.\2\[31:(\d+)\]" % (prefix, prefix, prefix), ctl):
        cuts.setdefault(m.group(2), set()).add(int(m.group(4)))

    slots, pools = {}, {}   # cut signature -> list of (slot_name, free_from_index)
    for v in sorted(first, key=lambda x: (first[x], x)):
        sig = tuple(sorted(cuts.get(v, ())))
        free = pools.setdefault(sig, [])
        pick = None
        for k, (nm, freeat) in enumerate(free):
            if freeat <= first[v]:
                pick = nm
                free[k] = (nm, last[v] + 1)
                break
        if pick is None:
            pick = "%s_s%d_%d" % (prefix, len(pools) - 1 if sig not in pools else list(pools).index(sig), len(free))
            pick = "%s_s%s_%d" % (prefix, "_".join(str(c) for c in sig) or "n", len(free))
            free.append((pick, last[v] + 1))
        slots[v] = pick
    free = [(nm, 0) for pl in pools.values() for nm, _ in pl]

    # Rewrite references and the struct.
    def rename(m):
        return "%s.%s" % (prefix, slots.get(m.group(1), m.group(1)))

    head = s[:ctl_start]
    tail = s[ctl_end:]
    ctl = _re.sub(r"%s\.(\w+)" % prefix, rename, ctl)

    kept = [l for l in mstruct.group(2).split("\n")
            if not _re.match(r"\s*bit<32>\s+(\w+);", l)
            or _re.match(r"\s*bit<32>\s+(\w+);", l).group(1) not in pool]
    kept += ["  bit<32> %s;" % nm for nm, _ in free]
    new_struct = mstruct.group(1) + "\n".join(kept) + mstruct.group(3)
    head = head[:mstruct.start()] + new_struct + head[mstruct.end():] if mstruct.start() < ctl_start else head

    return head + ctl + tail, len(pool), len(free)


def fK_liveness_slots(s):
    """Pool compute metadata into reused slots, per gress."""
    s, n_i, k_i = _pool_one_gress(s, "Ingress", "synapse_ingress_metadata_t", "meta")
    print("  fK: ingress %d compute fields -> %d slots" % (n_i, k_i))
    s, n_e, k_e = _pool_one_gress(s, "Egress", "synapse_egress_metadata_t", "eg_md")
    print("  fK: egress  %d compute fields -> %d slots" % (n_e, k_e))
    return s


def fS_shift_form_for_live_out(s):
    """A concat rotate cuts its operand; the shift form does not, and uses no hash unit either.
    Apply it only where the cut hurts: to rotates whose OPERAND is also copied whole into a
    deparsed header field. The two shifts are independent and stay in the enclosing action; the
    `|` depends on them, so it must go in its own action (bf-p4c: "action spanning multiple
    stages"), which is the one extra stage this form costs."""
    import re as _re
    live_out = set(_re.findall(r"hdr\.(?:cpu|egress_state|recirc)\.\w+ = (?:meta|eg_md)\.(\w+);", s))
    rot = _re.compile(r"^(\s*)(?:@in_hash \{ )?((?:meta|eg_md))\.(\w+) = \2\.(\w+)\[(\d+):0\] \+\+ \2\.\4\[31:(\d+)\];( \})?\s*$")
    act = _re.compile(r"(  action (\w+)\(\) \{)(.*?)(\n  \})", _re.S)
    decls = {"meta": [], "eg_md": []}
    extra, edits, n = {}, [], 0

    for am in act.finditer(s):
        head, aname, body, tail = am.group(1), am.group(2), am.group(3), am.group(4)
        lines = body.split("\n")
        ors = []
        changed = False
        for i, line in enumerate(lines):
            m = rot.match(line)
            if not m:
                continue
            indent, pfx, dst, src, lo = m.group(1), m.group(2), m.group(3), m.group(4), int(m.group(6))
            if src not in live_out:
                continue
            t1, t2 = "%s_rs1" % dst, "%s_rs2" % dst
            decls[pfx] += [t1, t2]
            lines[i] = ("%s%s.%s = %s.%s << %d;\n%s%s.%s = %s.%s >> %d;"
                        % (indent, pfx, t1, pfx, src, 32 - lo, indent, pfx, t2, pfx, src, lo))
            ors.append("%s.%s = %s.%s | %s.%s;" % (pfx, dst, pfx, t1, pfx, t2))
            changed = True
            n += 1
        if not changed:
            continue
        names, defs = [], []
        for k, stmt in enumerate(ors):
            nm = "%s_or%d" % (aname, k + 1)
            names.append(nm)
            defs.append("  action %s() {\n    %s\n  }\n" % (nm, stmt))
        extra[aname] = names
        edits.append((am.start(), am.end(), head + "\n".join(lines) + tail + "\n" + "".join(defs).rstrip("\n")))

    for a, b, rep in reversed(edits):
        s = s[:a] + rep + s[b:]

    def add(mm):
        indent, nm = mm.group(1), mm.group(2)
        if nm not in extra:
            return mm.group(0)
        return mm.group(0) + "".join("\n%s%s();" % (indent, e) for e in extra[nm])

    s = _re.sub(r"^([ \t]+)(\w+)\(\);", add, s, flags=_re.M)
    for pfx, struct in (("meta", "synapse_ingress_metadata_t"), ("eg_md", "synapse_egress_metadata_t")):
        if decls[pfx]:
            add_txt = "".join("  bit<32> %s;\n" % d for d in decls[pfx])
            s = _re.sub(r"(struct %s \{\n)" % struct, lambda m: m.group(1) + add_txt, s, count=1)
    assert n > 0, "fS: no live-out rotates"
    print("  fS: converted %d live-out rotates to shift form" % n)
    return s


def _cut_signatures(text, prefixes):
    """Every bit boundary any expression imposes on a field, propagated through plain copies.

    A boundary comes from a slice read/write `X[hi:lo]` (boundaries lo and hi+1) or a shift by n
    (boundary n). A plain copy `A = B;` is a whole-container move, so A and B must be laid out
    compatibly: union their boundary sets. Being generous here is safe -- an over-large signature
    only prevents a merge."""
    import re as _re

    field = r"(?:(?:%s)\.[\w.]+)" % "|".join(prefixes)
    bounds, parent = {}, {}

    def find(x):
        parent.setdefault(x, x)
        while parent[x] != x:
            parent[x] = parent[parent[x]]
            x = parent[x]
        return x

    def union(a, b):
        ra, rb = find(a), find(b)
        if ra != rb:
            parent[ra] = rb

    for m in _re.finditer(r"(%s)\[(\d+):(\d+)\]" % field, text):
        name, hi, lo = m.group(1), int(m.group(2)), int(m.group(3))
        b = bounds.setdefault(name, set())
        if lo:
            b.add(lo)
        if hi + 1 < 32:
            b.add(hi + 1)
    for m in _re.finditer(r"(%s)\s*(?:<<|>>)\s*(?:32w0x0*([0-9a-fA-F]+)|(\d+))" % field, text):
        n = int(m.group(2), 16) if m.group(2) else int(m.group(3))
        if 0 < n < 32:
            bounds.setdefault(m.group(1), set()).add(n)
    for m in _re.finditer(r"^\s*(?:@in_hash \{ )?(%s)\s*=\s*(%s)\s*;" % (field, field), text, _re.M):
        union(m.group(1), m.group(2))

    merged = {}
    for name in set(list(bounds) + list(parent)):
        merged.setdefault(find(name), set())
        merged[find(name)] |= bounds.get(name, set())
    return lambda name: tuple(sorted(merged.get(find(name), bounds.get(name, set()))))


def _pool_gress_v3(s, control_name, struct_name, prefix):
    import re as _re

    mstruct = _re.search(r"(struct %s \{)(.*?)(\n\})" % struct_name, s, _re.S)
    poolable = [n for w, n in _re.findall(r"bit<(\d+)>\s+(\w+);", mstruct.group(2))
                if w == "32" and _re.match(r"(rotate_left|op_)", n)]
    if not poolable:
        return s, 0, 0

    ctl_start = s.index("control %s(" % control_name)
    ctl_end = s.index("control %sDeparser(" % control_name)
    ctl = s[ctl_start:ctl_end]
    outside = s[:ctl_start] + s[ctl_end:]
    pool = {n for n in poolable if not _re.search(r"%s\.%s\b" % (prefix, _re.escape(n)), outside)}
    if not pool:
        return s, 0, 0

    sig_of = _cut_signatures(ctl, [prefix, "hdr"])

    bodies = {m.group(1): m.group(2) for m in _re.finditer(r"  action (\w+)\(\) \{(.*?)\n  \}", ctl, _re.S)}

    def defs_uses(text):
        d, u = set(), set()
        for st in text.split(";"):
            if "=" not in st:
                continue
            lhs, rhs = st.split("=", 1)
            d |= {n for n in _re.findall(r"%s\.(\w+)" % prefix, lhs) if n in pool}
            u |= {n for n in _re.findall(r"%s\.(\w+)" % prefix, rhs) if n in pool}
        return d, u

    apply_txt = _re.sub(r"//[^\n]*", "", ctl[ctl.index("  apply {"):])
    seq = []
    for m in _re.finditer(r"(\w+)\(\)\s*;|([\w.\[\]:]+\s*=\s*[^;]+);", apply_txt):
        body = bodies.get(m.group(1)) if m.group(1) else m.group(2)
        if body is not None:
            seq.append(defs_uses(body))

    first, last = {}, {}
    for i, (d, u) in enumerate(seq):
        for v in d | u:
            first.setdefault(v, i)
            last[v] = i
    for v in pool:
        if v not in first:
            first[v], last[v] = -1, 1 << 30

    slots, pools = {}, {}
    for v in sorted(first, key=lambda x: (first[x], x)):
        sig = sig_of("%s.%s" % (prefix, v))
        free = pools.setdefault(sig, [])
        pick = None
        for k, (nm, freeat) in enumerate(free):
            if freeat <= first[v]:
                pick, free[k] = nm, (nm, last[v] + 1)
                break
        if pick is None:
            pick = "%s_c%s_%d" % (prefix, "_".join(map(str, sig)) or "0", len(free))
            free.append((pick, last[v] + 1))
        slots[v] = pick

    allslots = [nm for pl in pools.values() for nm, _ in pl]
    ctl = _re.sub(r"%s\.(\w+)" % prefix, lambda m: "%s.%s" % (prefix, slots.get(m.group(1), m.group(1))), ctl)

    kept = [l for l in mstruct.group(2).split("\n")
            if not _re.match(r"\s*bit<32>\s+(\w+);", l)
            or _re.match(r"\s*bit<32>\s+(\w+);", l).group(1) not in pool]
    kept += ["  bit<32> %s;" % nm for nm in allslots]
    head = s[:ctl_start]
    head = head[:mstruct.start()] + mstruct.group(1) + "\n".join(kept) + mstruct.group(3) + head[mstruct.end():]
    return head + ctl + s[ctl_end:], len(pool), len(allslots)


def fK3_cut_signature_slots(s):
    """Pool compute metadata into reused slots, sharing a slot only between values with an
    IDENTICAL cut signature -- the property the ground truth gets for free because SipHash gives
    each word a fixed pair of rotations (v1 only 5 and 13, v3 only 8 and 7, v0/v2 only 16).
    Supersedes fK, which pooled cut-blind and so concentrated cuts instead of containing them."""
    s, n_i, k_i = _pool_gress_v3(s, "Ingress", "synapse_ingress_metadata_t", "meta")
    print("  fK3: ingress %d compute fields -> %d slots" % (n_i, k_i))
    s, n_e, k_e = _pool_gress_v3(s, "Egress", "synapse_egress_metadata_t", "eg_md")
    print("  fK3: egress  %d compute fields -> %d slots" % (n_e, k_e))
    return s


def fP_pin_slots(s):
    """Pin every pooled slot to a single 32-bit container.

    A deparsed header field is `exact_containers` and so stays whole; a metadata field has no such
    constraint, and when the allocator splits one into its boundary pieces every piece becomes a
    separate PHV source -- eight of them in `compute_rotate_left_177`, against a limit of two. This
    is what the ground truth's `@pa_container_size("ingress", "meta.a0", 32)` prevents, and why
    those pragmas look unnecessary there: its allocation is easy enough to find the same answer
    unaided (removing them changes nothing), but ours is not. Requires fK3: pinning the ~271
    unpooled fields is not possible, there are only 64 32-bit containers per pipe."""
    import re as _re
    n = 0
    out = []
    for gress, struct, prefix in (("ingress", "synapse_ingress_metadata_t", "meta"),
                                  ("egress", "synapse_egress_metadata_t", "eg_md")):
        m = _re.search(r"struct %s \{(.*?)\n\}" % struct, s, _re.S)
        for w, name in _re.findall(r"bit<(\d+)>\s+(\w+);", m.group(1)):
            # Only the slots that carry cuts, which is what the ground truth pins (its meta.a0..a3
            # and recirc_state.v0..v3, 8 per gress). Pinning the uncut slots too costs containers
            # that the header fields need: 38 pins starved hdr.hdr2.data2 and meta.time.
            if w == "32" and _re.match(r"%s_c[1-9][0-9_]*_\d+$" % prefix, name):
                out.append('@pa_container_size("%s", "%s.%s", 32)' % (gress, prefix, name))
                n += 1
    assert n > 0, "fP: no pooled slots found (apply K3 first)"
    anchor = "control Ingress("
    s = s.replace(anchor, "\n".join(out) + "\n" + anchor, 1)
    print("  fP: pinned %d slots to 32-bit containers" % n)
    return s


def _chain_to_header(s, control, mstruct_name, hstruct_name, prefix, hdr_type, hdr_field, end_anchor):
    import re as _re
    ms = _re.search(r"(struct %s \{)(.*?)(\n\})" % mstruct_name, s, _re.S)
    # Only the slots that carry cuts: those are the ones whose operands must stay whole.
    # ALL pooled slots, not only the ones a rotate cuts directly: cuts unify across the chain
    # (a = b ^ c forces b and c into compatible layouts), so leaving the "uncut" slots in metadata
    # just moves the failure onto them -- measured, 32 of gV1's 304 unallocated slices.
    moved = [n for w, n in _re.findall(r"bit<(\d+)>\s+(\w+);", ms.group(2))
             if w == "32" and _re.match(r"%s_c[0-9][0-9_]*_\d+$" % prefix, n)]
    if not moved:
        return s, 0

    kept = [l for l in ms.group(2).split("\n")
            if not _re.match(r"\s*bit<32>\s+(\w+);", l)
            or _re.match(r"\s*bit<32>\s+(\w+);", l).group(1) not in moved]
    s = s[:ms.start()] + ms.group(1) + "\n".join(kept) + ms.group(3) + s[ms.end():]

    decl = "header %s {\n%s}\n\n" % (hdr_type, "".join("  bit<32> %s;\n" % n for n in moved))
    # Before the headers struct that uses it: a header type must be declared before first use.
    s = _re.sub(r"(struct %s \{)" % hstruct_name, lambda m: decl + m.group(1), s, count=1)
    s = _re.sub(r"(struct %s \{\n)" % hstruct_name,
                lambda m: m.group(1) + "  %s %s;\n" % (hdr_type, hdr_field), s, count=1)

    ctl_start = s.index("control %s(" % control)
    ctl_end = s.index("control %sDeparser(" % control)
    ctl = s[ctl_start:ctl_end]
    for n in moved:
        ctl = _re.sub(r"\b%s\.%s\b" % (prefix, n), "hdr.%s.%s" % (hdr_field, n), ctl)
    # Valid for the whole control so the writes are live, invalid before the deparser so the
    # header never reaches the wire. It is still statically deparsed (pkt.emit(hdr) covers the
    # struct), which is what gives its fields exact_containers -- the property we are after.
    i = ctl.index("  apply {") + len("  apply {")
    ctl = ctl[:i] + "\n    hdr.%s.setValid();" % hdr_field + ctl[i:]
    j = ctl.index(end_anchor)
    ctl = ctl[:j] + "    hdr.%s.setInvalid();\n" % hdr_field + ctl[j:]
    return s[:ctl_start] + ctl + s[ctl_end:], len(moved)


def fV_chain_operands_in_header(s):
    """Give the chain's cut values the one property that makes the ground truth work.

    A rotate operand must occupy a single container, or its boundary pieces each become a PHV
    source (eight of them, against a limit of two). The ground truth gets this for free: its
    operands are `hdr.recirc_state.v0..v3`, header fields, which are `exact_containers` and so
    cannot be split. Ours are metadata, which the allocator splits freely, and there are far too
    many to pin with @pa_container_size (19 pins already starve hdr.hdr2.data2 and meta.time).

    So move them into a header. Requires fK3 first, to get the count down to something a header
    can hold."""
    s, n_i = _chain_to_header(s, "Ingress", "synapse_ingress_metadata_t", "synapse_ingress_headers_t",
                              "meta", "chain_h", "chain", "    forwarding_tbl.apply();")
    s, n_e = _chain_to_header(s, "Egress", "synapse_egress_metadata_t", "synapse_egress_headers_t",
                              "eg_md", "chain_e_h", "chain_e", "    hdr.egress_state.setInvalid();")
    print("  fV: moved %d ingress and %d egress chain values into headers" % (n_i, n_e))
    return s


def fJ_parser_protocol_dispatch(s):
    """CORRECTNESS. Synapse's ingress parser inverts the IPv4 protocol dispatch: protocol 0x11
    (UDP) reaches `pkt.extract(hdr.hdr2)` (160 bits, the TCP header) and protocol 0x06 (TCP) falls
    through to `pkt.extract(hdr.hdr3)` (64 bits, UDP). The nested state also re-tests the same
    field for a value it cannot hold (inside the ==0x11 branch it asks ==0x06). A TCP packet
    therefore never gets a TCP header and the whole cookie path is unreachable -- the model shows
    `hdr.hdr2.$valid == 1 not matched` and the SYN routed to the server untouched.

    Minimal repair keeping every state reachable: send 0x06 down the branch that extracts hdr2,
    and let everything else fall through to the one that extracts hdr3."""
    a = """  state parser_5_0 {
    transition select (hdr.hdr1.data2[23:16]) {
      8w0x11: parser_6;
      default: parser_201;
    }
  }"""
    b = """  state parser_5_0 {
    transition select (hdr.hdr1.data2[23:16]) {
      8w0x06: parser_6;
      default: parser_201;
    }
  }"""
    c = """  state parser_6_0 {
    transition select (hdr.hdr1.data2[23:16]) {
      8w0x06: parser_9;
      default: parser_10;
    }
  }"""
    d = """  state parser_6_0 {
    transition select (hdr.hdr1.data2[23:16]) {
      8w0x11: parser_9;
      default: parser_10;
    }
  }"""
    assert s.count(a) == 1, "fJ: parser_5_0"
    assert s.count(c) == 1, "fJ: parser_6_0"
    s = s.replace(a, b).replace(c, d)
    print("  fJ: TCP now reaches hdr2, UDP reaches hdr3")
    return s


def fCS_deparser_checksums(s):
    """CORRECTNESS. Synapse emits no deparser checksums: its IngressDeparser is a bare
    `pkt.emit(hdr)`. The crafted SYN-ACK therefore carries a stale TCP checksum -- the model test
    shows the packet byte-identical to the expected one except for those two bytes.

    The ground truth recomputes both in its deparser, and its header layout gives each checksum its
    own field. Ours buries them inside wider guessed fields, so this is a re-cut (same family as
    fix 1) plus two Checksum() externs:
      IPv4 bytes 9..11 are protocol + checksum -> split hdr1.data2 into 8 + 16
      TCP  bytes 14..19 are window + checksum + urgent -> split hdr2.data4 into 16 + 16 + 16

    Guarded on a flag set by the one block that rewrites the packet: recomputing unconditionally
    would corrupt pass-through packets, whose payload the deparser cannot see and whose TCP length
    is not the 20 assumed here."""
    # --- re-cut the two headers -----------------------------------------------------------------
    s = sub1(s, "  bit<40> data1;\n  bit<24> data2;\n",
             "  bit<40> data1;\n  bit<8> data2;\n  bit<16> data2b;\n", "fCS ipv4 layout")
    s = sub1(s, "  bit<16> data3;\n  bit<48> data4;\n",
             "  bit<16> data3;\n  bit<16> data4;\n  bit<16> data4b;\n  bit<16> data4c;\n", "fCS tcp layout")
    n = s.count("hdr.hdr1.data2[23:16]")
    assert n == 2, "fCS: protocol reads = %d" % n
    s = s.replace("hdr.hdr1.data2[23:16]", "hdr.hdr1.data2")

    # --- a flag, so only rewritten packets are recomputed ----------------------------------------
    s = sub1(s, "  bit<1> to_egress;", "  bit<1> to_egress;\n  bit<1> redo_checksum;\n  bit<16> tcp_len;", "fCS flag decl")
    s = sub1(s, "    meta.dev = 0;", "    meta.dev = 0;\n    meta.redo_checksum = 0;", "fCS flag init")
    s = sub1(s, "        hdr.hdr1.data0 = 8w0x45 ++ hdr.hdr1.data0[23:16] ++ 8w0x00 ++ 8w0x28;",
             "        hdr.hdr1.data0 = 8w0x45 ++ hdr.hdr1.data0[23:16] ++ 8w0x00 ++ 8w0x28;\n"
             "        meta.redo_checksum = 1;\n        meta.tcp_len = 20;", "fCS flag set")

    # --- the deparser ----------------------------------------------------------------------------
    old = """control IngressDeparser(
  packet_out pkt,
  inout synapse_ingress_headers_t hdr,
  in    synapse_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {

  apply {
    pkt.emit(hdr);
  }
}"""
    new_dep = """control IngressDeparser(
  packet_out pkt,
  inout synapse_ingress_headers_t hdr,
  in    synapse_ingress_metadata_t meta,
  in    ingress_intrinsic_metadata_for_deparser_t ig_dprsr_md
) {
  Checksum() ipv4_checksum;
  Checksum() tcp_checksum;

  apply {
    if (meta.redo_checksum == 1) {
      hdr.hdr1.data2b = ipv4_checksum.update({
        hdr.hdr1.data0, hdr.hdr1.data1, hdr.hdr1.data2, hdr.hdr1.data3, hdr.hdr1.data4
      });
      // Pseudo-header (src, dst, zero, protocol, TCP length) then the TCP header without its own
      // checksum. The rewritten packet is always a bare 20-byte segment, so the length is 20 and
      // there is no payload to cover.
      hdr.hdr2.data4b = tcp_checksum.update({
        hdr.hdr1.data3, hdr.hdr1.data4, 8w0, hdr.hdr1.data2, meta.tcp_len,
        hdr.hdr2.data0, hdr.hdr2.data1, hdr.hdr2.data2, hdr.hdr2.data3,
        hdr.hdr2.data4, hdr.hdr2.data4c
      });
    }
    pkt.emit(hdr);
  }
}"""
    s = sub1(s, old, new_dep, "fCS deparser")
    print("  fCS: split the two checksum fields out and added deparser checksums")
    return s


FIXES = {"1": f1_header_recut, "2": f2_identity_concat, "3": f3_natural_width,
         "4": f4_in_hash_rotates, "A": fA_cpu_header, "B": fB_whole_field_key, "C": fC_device_in_hash, "D": fD_header_write_in_hash, "E": fE_stage_header_read, "G": fG_one_hash_per_action, "A2": fA2_cpu_write_in_hash, "H": fH_egress_parses_recirc, "W": fW_deparsed_writes_in_hash, "K": fK_liveness_slots, "S": fS_shift_form_for_live_out, "K3": fK3_cut_signature_slots, "P": fP_pin_slots, "V": fV_chain_operands_in_header, "J": fJ_parser_protocol_dispatch, "CS": fCS_deparser_checksums}

if __name__ == "__main__":
    src, dst, names = sys.argv[1], sys.argv[2], sys.argv[3:]
    text = open(src).read()
    for name in names:
        print("applying %s" % name)
        text = FIXES[name](text)
    open(dst, "w").write(text)
    print("wrote %s (%s)" % (dst, " ".join(names) or "unmodified"))
