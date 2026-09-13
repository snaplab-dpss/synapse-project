"""A program's budget sheet from its P4 and bf-p4c's outputs (compiled with --verbose 2): per gress,
the hash-unit and ALU statements, the state words and the packet inputs the chain reads on the
ALU, and per stage the tables, the hash-distribution units and the 32-bit containers. Two sheets
side by side say which resource a gap belongs to. usage: budget.py NAME FILE.p4 OUTDIR [NAME FILE.p4 OUTDIR]"""
import re, sys
from collections import defaultdict

WORD = r"(?:hdr\.st|hdr\.recirc_state|meta|eg_md)\.(?:s32_\d+|[av]\d+)"


def p4_sheet(path):
    t = open(path).read()
    sheet = {}
    ctrls = [(m.start(), m.group(1)) for m in re.finditer(r"^control (\w+)\(", t, re.M)]
    for k, (start, name) in enumerate(ctrls):
        end = ctrls[k + 1][0] if k + 1 < len(ctrls) else len(t)
        body = t[start:end]
        gress = "ingress" if "Ingress" in name and "Deparser" not in name else "egress" if "Egress" in name and "Deparser" not in name else None
        if gress is None or "Parser" in name:
            continue
        acts = {m.group(1): m.group(2) for m in re.finditer(r"  action (\w+)\(\) \{(.*?)\n  \}", body, re.S)}
        hash_stmts = re.findall(r"@in_hash \{ ([^;]+); \}", body)
        rot = sum(1 for s in hash_stmts if "++" in s)
        alu = 0
        words, alu_in, hash_in = set(), set(), set()
        for m in re.finditer(r"(@in_hash \{ )?(" + WORD + r"|[\w.]+)\s*=\s*([^;]+);", body):
            dst, rhs = m.group(2), m.group(3)
            if re.fullmatch(WORD, dst):
                words.add(dst)
            if re.fullmatch(WORD, dst) or re.search(WORD, rhs):
                if m.group(1):
                    hash_in.update(f for f in re.findall(r"hdr\.\w+\.\w+", rhs) if not re.fullmatch(WORD, f))
                else:
                    alu += 1
                    alu_in.update(f for f in re.findall(r"hdr\.\w+\.\w+", rhs) if not re.fullmatch(WORD, f))
        sheet[gress] = dict(hash=len(hash_stmts), rotates=rot, entries=len(hash_stmts) - rot, alu=alu, words=len(words),
                            alu_inputs=sorted(alu_in), hash_inputs=sorted(hash_in), actions=len(acts))
    return sheet


def bfa_sheet(outdir):
    bfa = open(outdir + "/pipe/" + [f for f in __import__("os").listdir(outdir + "/pipe") if f.endswith(".bfa")][0]).read().splitlines()
    stage = gress = None
    tables = defaultdict(lambda: defaultdict(int))
    units = defaultdict(lambda: defaultdict(int))
    for l in bfa:
        m = re.match(r"stage (\d+) (ingress|egress):", l)
        if m:
            stage, gress = int(m.group(1)), m.group(2)
            continue
        m = re.match(r"  ([a-z_]+) (\S+) \d+:", l)
        if m and stage is not None:
            tables[stage][gress] += 1
        if stage is not None and re.match(r"      \d+: \{ hash: \d+, mask:", l):
            units[stage][gress] += 1
    return tables, units


def resources_sheet(outdir):
    t = open(outdir + "/pipe/logs/mau.resources.log").read()
    rows = re.findall(r"^\|\s*(\d+)\s*\|(?:[^|]*\|){3}\s*(\d+)\s*\|", t, re.M)
    return {int(s): int(u) for s, u in rows}


def phv_sheet(outdir):
    import glob
    f = sorted(glob.glob(outdir + "/pipe/logs/phv_allocation_summary_*.log"))[-1]
    cur = None
    conts = defaultdict(set)
    for r in open(f).read().splitlines():
        m = re.match(r"\|(\w+)\s*\|(\w?)\s*\|\[([^\]]*)\]\s*\|(\S+)\s*\|", r)
        if not m:
            continue
        c, g, sl, fld = m.groups()
        if c:
            cur = (c, g)
        if cur and cur[0].startswith(("W", "MW", "DW")) and "W" in cur[0]:
            conts[cur].add(re.sub(r"\[.*", "", fld))
    return conts


def show(name, p4, outdir):
    print(f"=== {name}: {p4}")
    for gress, s in p4_sheet(p4).items():
        print(f"  {gress:8s} hash statements {s['hash']:3d} (rotates {s['rotates']}, entries {s['entries']}), ALU statements {s['alu']:3d}, "
              f"state words {s['words']:2d}, actions {s['actions']}")
        print(f"           packet inputs read on the ALU next to a word: {s['alu_inputs']}")
        print(f"           packet inputs read by the hash unit: {s['hash_inputs']}")
    try:
        tables, units = bfa_sheet(outdir)
        res = resources_sheet(outdir)
        stages = sorted(set(tables) | set(res))
        print(f"  stages used: {len(stages)} (0..{max(stages)})")
        print("  stage: tables ingress+egress / hash-dist units (resource log, physical)")
        print("  " + " ".join(f"{s}:{tables[s]['ingress']}+{tables[s]['egress']}/{res.get(s, '-')}" for s in stages))
        print(f"  hash-dist units total (resource log): {sum(res.values())}")
    except (FileNotFoundError, IndexError) as e:
        print("  (no assembly / resource log:", e, ")")
    try:
        conts = phv_sheet(outdir)
        for g in ("I", "E"):
            normal = {c for (c, gg) in conts if gg == g and c.startswith("W")}
            mocha = {c for (c, gg) in conts if gg == g and c.startswith("MW")}
            words = {c: sorted(f for f in conts[(c, g)] if re.search(r"(hdr\.st|recirc_state)\.", f)) for c in normal | mocha}
            words = {c: f for c, f in words.items() if f}
            groups = defaultdict(int)
            for c in words:
                groups[int(re.sub(r"\D", "", c)) // 12 if c.startswith("W") else "M" + str(int(re.sub(r"\D", "", c)) // 4)] += 1
            print(f"  PHV {'ingress' if g == 'I' else 'egress'}: 32-bit containers used {len(normal)} normal + {len(mocha)} mocha; "
                  f"state words in {len(words)} containers, by group {dict(groups)}")
    except (FileNotFoundError, IndexError) as e:
        print("  (no PHV summary:", e, ")")


args = sys.argv[1:]
while args:
    show(*args[:3])
    args = args[3:]
