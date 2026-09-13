"""Re-home a skeleton's call sequence in ONE pool of header words by liveness, with one rule: a word
D is never written from a word S with two different rotations (aligned ALU ops = 0, byte rotates
by their amount); hash-unit statements are free. State header parsed on port 6, init sets valid.
usage: rehome3.py SRC DST PORT COMMENT [--no-rule]"""
import re, sys
src_path, out_path, port, comment = sys.argv[1:5]
flags = set(sys.argv[5:])
t = open(src_path).read()
m = re.search(r"apply \{ init\(\); if \(ig_intr_md\.ingress_port == " + port + r"\) \{ (.*?) \} fin\(\); \}", t)
calls = re.findall(r"\b((?:compute|select)_\w+)\(\);", m.group(1))
acts = {mm.group(1): mm.group(2) for mm in re.finditer(r"  action ((?:compute|select)_\w+)\(\) \{(.*?)\n  \}", t, re.S)}
stmts = []
for c in calls:
    for line in acts[c].strip().splitlines():
        line = line.strip().rstrip(";"); in_hash = line.startswith("@in_hash")
        core = line.replace("@in_hash { ", "").rstrip(" }").rstrip(";") if in_hash else line
        dest, rhs = [x.strip() for x in core.split(" = ", 1)]
        srcs = re.findall(r"hdr\.st\.(s32_\d+)", rhs)
        rots = {}
        for s in set(srcs):
            r = 0
            mm = re.search(r"hdr\.st\." + s + r"\[(\d+):0\] \+\+ hdr\.st\." + s + r"\[31:(\d+)\]", rhs)
            if mm: r = 31 - int(mm.group(1))
            mm = re.search(r"hdr\.st\." + s + r" << (\d+)", rhs)
            if mm: r = int(mm.group(1))
            mm = re.search(r"hdr\.st\." + s + r" >> (\d+)", rhs)
            if mm: r = 32 - int(mm.group(1))
            rots[s] = r
        stmts.append({"action": c, "dest": re.findall(r"hdr\.st\.(s32_\d+)", dest), "rhs": rhs, "in_hash": in_hash, "srcs": srcs, "rots": rots})
act_end = {}
for i, st in enumerate(stmts): act_end[st["action"]] = i
values = []; last_def = {}
for i, st in enumerate(stmts):
    for s in st["srcs"]:
        if s not in last_def:
            values.append({"def": -1, "last_read": i}); last_def[s] = len(values) - 1
        values[last_def[s]]["last_read"] = max(values[last_def[s]]["last_read"], i)
    st["src_vals"] = {s: last_def[s] for s in st["srcs"]}
    if st["dest"]:
        values.append({"def": i, "last_read": act_end[st["action"]]}); last_def[st["dest"][0]] = len(values) - 1; st["val"] = len(values) - 1
    else: st["val"] = None
# allocation in definition order; a value's sources are earlier values, already homed
slots = []  # busy_until
rot_of = {}  # (D, S) -> rotation
order = sorted(range(len(values)), key=lambda vi: values[vi]["def"])
for vi in order:
    v = values[vi]
    st = stmts[v["def"]] if v["def"] >= 0 else None
    needs = {}
    if st and not st["in_hash"] and "--no-rule" not in flags:
        for s, sv in st["src_vals"].items(): needs[values[sv]["slot"]] = st["rots"][s]
    chosen = None
    for d, busy in enumerate(slots):
        if busy >= v["def"]: continue
        if any(rot_of.get((d, s), r) != r for s, r in needs.items()): continue
        chosen = d; break
    if chosen is None: slots.append(-1); chosen = len(slots) - 1
    slots[chosen] = v["last_read"]; v["slot"] = chosen
    for s, r in needs.items(): rot_of[(chosen, s)] = r
name = lambda vi: f"hdr.st.w{values[vi]['slot']}"
new_acts = {}
for st in stmts:
    rhs = st["rhs"]
    for s, sv in st["src_vals"].items(): rhs = re.sub(r"hdr\.st\." + s + r"\b", name(sv), rhs)
    line = f"{name(st['val'])} = {rhs};" if st["val"] is not None else f"{rhs};"
    if st["in_hash"]: line = "@in_hash { " + line + " }"
    new_acts.setdefault(st["action"], []).append(line)
out = t
for c, lines in new_acts.items():
    out = re.sub(r"  action " + c + r"\(\) \{.*?\n  \}", "  action " + c + "() {\n" + "\n".join("    " + l for l in lines) + "\n  }", out, flags=re.S)
out = re.sub(r"header state_h \{.*?\n\}", "header state_h {\n" + "".join(f"  bit<32> w{i};\n" for i in range(len(slots))) + "".join(f"  bit<32> s32_{i};\n" for i in range(11)) + "}", out, flags=re.S)
old = "pkt.extract(hdr.recirc); transition accept; }"
assert out.count(old) == 1
out = out.replace(old, "pkt.extract(hdr.recirc); transition select(ig_intr_md.ingress_port) { 6: parse_st; default: accept; } }\n  state parse_st { pkt.extract(hdr.st); transition accept; }")
out = re.sub(r"action init\(\) \{[^}]*\}", "action init() { hdr.st.setValid(); }", out, count=1)
out = out.replace(t.splitlines()[0], "// " + comment, 1)
open(out_path, "w").write(out)
conf = sum(1 for (d, s), r in rot_of.items() if False)
print(f"{out_path.split('/')[-1]}: words={len(slots)} values={len(values)}")
