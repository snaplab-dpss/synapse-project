"""Re-home a skeleton's call sequence in the ground truth's discipline (see scy-gt6): adds and hash
rotates write pinned metadata (a), xors write pinned header words (v), byte rotates and moves take
the kind their consumers need, a copy is inserted where a value is needed as both kinds."""
import re, sys
src_path, out_path, port, comment = sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4]
t = open(src_path).read()
m = re.search(r"apply \{ init\(\); if \(ig_intr_md\.ingress_port == " + port + r"\) \{ (.*?) \} fin\(\); \}", t)
calls = re.findall(r"\b((?:compute|select)_\w+)\(\);", m.group(1))
acts = {mm.group(1): mm.group(2) for mm in re.finditer(r"  action ((?:compute|select)_\w+)\(\) \{(.*?)\n  \}", t, re.S)}
stmts = []
for c in calls:
    for line in acts[c].strip().splitlines():
        line = line.strip().rstrip(";"); in_hash = line.startswith("@in_hash")
        core = line.replace("@in_hash { ", "").rstrip(" }") if in_hash else line
        dest, rhs = [x.strip() for x in core.split(" = ", 1)]
        kind = ("hashrot" if in_hash and "++" in rhs else "hashxor" if in_hash and "^" in rhs else "load" if in_hash else
                "rot" if "++" in rhs else "add" if (" + " in rhs or " - " in rhs) else "xor" if ("^" in rhs or "|" in rhs) else "move")
        stmts.append({"action": c, "dest": re.findall(r"hdr\.st\.(s32_\d+)", dest), "rhs": rhs.rstrip(";"), "in_hash": in_hash, "kind": kind, "srcs": re.findall(r"hdr\.st\.(s32_\d+)", rhs)})
act_end = {}
for i, st in enumerate(stmts): act_end[st["action"]] = i
values = []; last_def = {}
for i, st in enumerate(stmts):
    for s in st["srcs"]:
        if s not in last_def:
            values.append({"kind": "carried", "def": -1, "last_read": i}); last_def[s] = len(values) - 1
        values[last_def[s]]["last_read"] = max(values[last_def[s]]["last_read"], i)
    st["src_vals"] = [last_def[s] for s in st["srcs"]]
    if st["dest"]:
        values.append({"kind": st["kind"], "def": i, "last_read": act_end[st["action"]]}); last_def[st["dest"][0]] = len(values) - 1; st["val"] = len(values) - 1
    else: st["val"] = None
fixed = {"add": "a", "hashrot": "a", "xor": "v", "hashxor": "v"}
for v in values: v["pool"] = fixed.get(v["kind"])
changed = True
while changed:
    changed = False
    for st in stmts:
        svs = st["src_vals"]
        if st["kind"] == "add":
            for sv in svs:
                if values[sv]["pool"] is None: values[sv]["pool"] = "v"; changed = True
        if st["kind"] in ("xor", "hashxor") and len(svs) == 2:
            a, b = [values[sv]["pool"] for sv in svs]
            if a is None and b is not None: values[svs[0]]["pool"] = b; changed = True
            if b is None and a is not None: values[svs[1]]["pool"] = a; changed = True
for v in values:
    if v["pool"] is None: v["pool"] = "v"
copies = {}
for st in stmts:
    svs = st["src_vals"]; ks = [values[sv]["pool"] for sv in svs]
    if st["kind"] in ("xor", "hashxor") and len(ks) == 2 and ks[0] != ks[1]:
        vi = svs[0] if ks[0] == "v" else svs[1]; copies.setdefault((vi, "a"), None); st.setdefault("use_copy", []).append((vi, "a"))
    if st["kind"] == "add":
        for sv, k in zip(svs, ks):
            if k == "a": copies.setdefault((sv, "v"), None); st.setdefault("use_copy", []).append((sv, "v"))
names = {"a": [], "v": []}; busy = {}
allocs = [(v["def"], v["last_read"], v["pool"], ("val", vi)) for vi, v in enumerate(values)]
allocs += [(values[vi]["def"] + 0.5, values[vi]["last_read"], k, ("copy", vi, k)) for (vi, k) in copies]
for d, lr, p, who in sorted(allocs, key=lambda x: x[0]):
    chosen = None
    for n in names[p]:
        if busy[n] < d: chosen = n; break
    if chosen is None: chosen = f"{p}{len(names[p])}"; names[p].append(chosen)
    busy[chosen] = lr; name = ("meta." if p == "a" else "hdr.st.") + chosen
    if who[0] == "val": values[who[1]]["name"] = name
    else: copies[(who[1], who[2])] = name
def src_name(st, sv):
    for need in st.get("use_copy", []):
        if need[0] == sv: return copies[need]
    return values[sv]["name"]
new_acts = {}; copy_actions = {}; init_extra = []
for st in stmts:
    rhs = st["rhs"]
    for s, sv in zip(st["srcs"], st["src_vals"]): rhs = re.sub(r"hdr\.st\." + s + r"\b", src_name(st, sv), rhs)
    line = f"{values[st['val']]['name']} = {rhs};" if st["val"] is not None else f"{rhs};"
    if st["in_hash"]: line = "@in_hash { " + line + " }"
    new_acts.setdefault(st["action"], []).append(line)
for (vi, k), name in copies.items():
    srcv = values[vi]; line = f"{name} = {srcv['name']};"
    if srcv["def"] < 0: init_extra.append(line)
    else: copy_actions.setdefault(stmts[srcv["def"]]["action"], []).append(line)
out = t
for c, lines in new_acts.items():
    body = "  action " + c + "() {\n" + "\n".join("    " + l for l in lines) + "\n  }"
    if c in copy_actions: body += "\n  action " + c + "_cp() {\n" + "\n".join("    " + l for l in copy_actions[c]) + "\n  }"
    out = re.sub(r"  action " + c + r"\(\) \{.*?\n  \}", body, out, flags=re.S)
seq = []
for c in calls:
    seq.append(f"{c}();")
    if c in copy_actions: seq.append(f"{c}_cp();")
out = out.replace(m.group(0), "apply { init(); if (ig_intr_md.ingress_port == " + port + ") { " + " ".join(seq) + " } fin(); }")
out = re.sub(r"header state_h \{.*?\n\}", "header state_h {\n" + "".join(f"  bit<32> {n};\n" for n in names["v"]) + "".join(f"  bit<32> s32_{i};\n" for i in range(11)) + "}", out, flags=re.S)
out = re.sub(r"struct meta_t \{ .*? \}", "struct meta_t { " + " ".join(f"bit<32> {n};" for n in names["a"]) + " bit<32> cond_operand_90_0_out; bit<32> op_xor_345_out; }", out)
prag = "".join(f'@pa_container_size("ingress", "meta.{n}", 32)\n' for n in names["a"]) + "".join(f'@pa_container_size("ingress", "hdr.st.{n}", 32)\n' for n in names["v"])
out = out.replace("control Ig(", prag + "control Ig(", 1)
carried = [v for v in values if v["kind"] == "carried"]
out = out.replace("action init() { hdr.st.setValid(); }", "action init() { hdr.st.setValid(); " + " ".join(f"{v['name']} = hdr.hdr1.data{i % 5};" for i, v in enumerate(carried)) + " " + " ".join(init_extra) + " }", 1)
out = out.replace(t.splitlines()[0], "// " + comment, 1)
open(out_path, "w").write(out)
print(f"pools a={len(names['a'])} v={len(names['v'])} copies={len(copies)} carried={len(carried)}")
