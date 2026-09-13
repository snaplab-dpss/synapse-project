"""Per gress: pairs (destination word, source word) written by ALU statements with more than one
rotation (aligned = 0; S[hi:0] ++ S[31:hi+1] = rotate left by 31-hi; shifts by their amount).
Hash-unit statements do not count. usage: rotcheck.py FILE [--all]"""
import re, sys
t = open(sys.argv[1]).read()
WORD = r"(?:hdr\.st|hdr\.recirc_state|meta|eg_md)\.(?:s32_\d+|[avw]\d+|x12)"
def gress_of(pos):
    ctrls = [(m.start(), m.group(1)) for m in re.finditer(r"^control (\w+)", t, re.M)]
    name = [n for p, n in ctrls if p <= pos][-1] if ctrls else "?"
    return "E" if "gress" in name and name.startswith("Eg") or "Egress" in name else "I"
pairs = {}
for m in re.finditer(r"(@in_hash \{ )?(" + WORD + r") = ([^;]+);", t):
    if m.group(1): continue
    dst, rhs = m.group(2), m.group(3)
    g = gress_of(m.start())
    dst = dst.split(".")[-1]
    for s in set(re.findall(WORD, rhs)):
        sn = s.split(".")[-1]
        rot = 0
        mm = re.search(re.escape(s) + r"\[(\d+):0\] \+\+ " + re.escape(s) + r"\[31:(\d+)\]", rhs)
        if mm: rot = 31 - int(mm.group(1))
        amount = lambda a: int(a, 16) if a.startswith("0x") else int(a)
        mm = re.search(re.escape(s) + r" << (?:\d+w)?(0x[0-9a-fA-F]+|\d+)", rhs)
        if mm: rot = amount(mm.group(1)) % 32
        mm = re.search(re.escape(s) + r" >> (?:\d+w)?(0x[0-9a-fA-F]+|\d+)", rhs)
        if mm: rot = (32 - amount(mm.group(1))) % 32
        pairs.setdefault((g, dst, sn), set()).add(rot)
bad = {k: v for k, v in pairs.items() if len(v) > 1}
print(f"{sys.argv[1].split('/')[-1]:28s} pairs={len(pairs):3d} conflicting={len(bad):2d}", "  ".join(f"{g}:{d}<-{s}{sorted(r)}" for (g, d, s), r in sorted(bad.items())))
