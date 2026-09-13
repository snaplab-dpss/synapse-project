"""Replay plan_value_homes' slot allocation offline from a `[homes]` dump, with a choice of
heuristics, to see how many words each gress needs. Sources are recovered from the dump: the
value that held the named slot at the reader's definition time. usage: realloc.py DUMP [heuristic...]"""
import re, sys
INF = 1 << 20
lines = open(sys.argv[1]).read().splitlines()
paths = []; cur = None
for l in lines:
    if l.startswith("[homes] path of"): cur = []; paths.append(cur); continue
    if cur is not None and l.startswith("  hdr.st."): cur.append(l)
    elif cur is not None and not l.startswith("  "): cur = None
def parse(p):
    vals = []
    for l in p:
        m = re.match(r"  (hdr\.st\.\S+) <- (\S+)( \(\S+\))?.*?\[(\d+), (\w+)\]( fixed)?( egress)?(.*)", l)
        slot, op, sym, fr, to, fixed, eg, rest = m.groups()
        srcs = re.findall(r"(hdr\.st\.\S+):(\d+)", rest)
        vals.append(dict(slot=slot, op=op, fr=int(fr), to=INF if to == "end" else int(to), fixed=bool(fixed), eg=bool(eg), srcs=[(s, int(r)) for s, r in srcs]))
    # resolve sources to ops: the value in that slot with the largest from <= reader.from (not itself)
    by_slot = {}
    for v in vals: by_slot.setdefault(v["slot"], []).append(v)
    for v in vals:
        res = []
        for s, r in v["srcs"]:
            cands = [w for w in by_slot.get(s, []) if w is not v and w["fr"] <= v["fr"] and w["eg"] == v["eg"] or (w is not v and w["fr"] <= v["fr"] and s == v["slot"])]
            cands = [w for w in by_slot.get(s, []) if w is not v and w["fr"] <= v["fr"]]
            if cands: res.append((max(cands, key=lambda w: w["fr"])["op"], r))
        v["src_ops"] = res
    return vals
all_paths = [parse(p) for p in paths if p]
def simulate(heur):
    slot_of = {}      # op -> slot index
    rot = {}          # (eg, D, S) -> rotation
    readers = {}      # op -> [(reader op, rot, eg)]
    nslots = 0
    per_gress = {False: set(), True: set()}
    for vals in all_paths:
        for v in vals:
            for s, r in v["src_ops"]: readers.setdefault(s, []).append((v["op"], r, v["eg"]))
        busy = {}  # slot -> busy_until on this path (values already fixed count as fixed ranges)
        fixed_ranges = {}
        order = sorted(vals, key=lambda v: v["fr"])
        for v in order:
            if v["op"] in slot_of:
                d = slot_of[v["op"]]; fixed_ranges.setdefault(d, []).append((v["fr"], v["to"]))
                per_gress[v["eg"]].add(d)
        for v in order:
            if v["op"] in slot_of: continue
            pairs = [(v["eg"], None, slot_of[s], r) for s, r in v["src_ops"] if s in slot_of]
            pairs += [(eg, slot_of[rd], None, r) for rd, r, eg in readers.get(v["op"], []) if rd in slot_of]
            def key(pair, own): eg, d, s, r = pair; return (eg, own if d is None else d, own if s is None else s)
            def compatible(d): return all(rot.get(key(p, d), p[3]) == p[3] for p in pairs)
            def new_pairs(d): return sum(1 for p in pairs if key(p, d) not in rot)
            cands = []
            inplace = slot_of.get(v["op"][:-3] + "_shl") if v["op"].endswith("_or") else None
            for d in range(nslots):
                free = busy.get(d, -1) < v["fr"] and all(t < v["fr"] or v["to"] < f for f, t in fixed_ranges.get(d, []))
                if inplace is not None and d == inplace: free = busy.get(d, -1) <= v["fr"] and all(t <= v["fr"] or v["to"] < f for f, t in fixed_ranges.get(d, []))
                if free and compatible(d): cands.append(d)
            if inplace is not None and inplace in cands: chosen = inplace
            elif not cands: chosen = nslots; nslots += 1
            else: chosen = heur(cands, v, busy, rot, new_pairs, key, pairs)
            busy[chosen] = v["to"]; slot_of[v["op"]] = chosen; per_gress[v["eg"]].add(chosen)
            for p in pairs: rot[key(p, chosen)] = p[3]
    return nslots, len(per_gress[False]), len(per_gress[True])
H = {
 "first-fit": lambda c, v, busy, rot, npairs, key, pairs: c[0],
 "fewest-new-pairs": lambda c, v, busy, rot, npairs, key, pairs: min(c, key=lambda d: (npairs(d), d)),
 "lru": lambda c, v, busy, rot, npairs, key, pairs: min(c, key=lambda d: (busy.get(d, -1), d)),
 "mru": lambda c, v, busy, rot, npairs, key, pairs: max(c, key=lambda d: (busy.get(d, -1), -d)),
 "fewest-new-then-mru": lambda c, v, busy, rot, npairs, key, pairs: min(c, key=lambda d: (npairs(d), -busy.get(d, -1), d)),
 "fewest-new-then-lru": lambda c, v, busy, rot, npairs, key, pairs: min(c, key=lambda d: (npairs(d), busy.get(d, -1), d)),
 "most-known-pairs": lambda c, v, busy, rot, npairs, key, pairs: min(c, key=lambda d: (-(len(pairs) - npairs(d)), d)),
}
for name in (sys.argv[2:] or H):
    print(f"{name:22s} slots total {simulate(H[name])[0]:2d}  ingress {simulate(H[name])[1]:2d}  egress {simulate(H[name])[2]:2d}")
