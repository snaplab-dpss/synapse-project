"""Re-home a skeleton's call sequence under the ground truth's alternation: every ALU statement reads
values of one kind and writes the other (xor, add, byte rotate, move); hash-unit statements are free
unless --hash-va (hash rotates read v, write a, as the ground truth does). A copy (its own action after
the definer's) resolves operands of different kinds. Both kinds are header words unless --meta-a.
The state header is also extracted by the parser on port 6, and init only sets it valid.

usage: rehome2.py SRC DST PORT COMMENT [--hash-va] [--meta-a] [--pragmas]"""
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
        kind = ("hashrot" if in_hash and "++" in rhs else "hashxor" if in_hash and "^" in rhs else "load" if in_hash else
                "rot" if "++" in rhs else "add" if (" + " in rhs or " - " in rhs) else "xor" if ("^" in rhs or "|" in rhs) else "move")
        stmts.append({"action": c, "dest": re.findall(r"hdr\.st\.(s32_\d+)", dest), "rhs": rhs, "in_hash": in_hash, "kind": kind,
                      "srcs": re.findall(r"hdr\.st\.(s32_\d+)", rhs)})
act_end = {}
for i, st in enumerate(stmts): act_end[st["action"]] = i
# values: carried (read before written) or defined by a statement
values = []; last_def = {}
for i, st in enumerate(stmts):
    for s in st["srcs"]:
        if s not in last_def:
            values.append({"kind": "carried", "def": -1, "last_read": i, "pool": None}); last_def[s] = len(values) - 1
        values[last_def[s]]["last_read"] = max(values[last_def[s]]["last_read"], i)
    st["src_vals"] = [last_def[s] for s in st["srcs"]]
    if st["dest"]:
        values.append({"kind": st["kind"], "def": i, "last_read": act_end[st["action"]], "pool": None}); last_def[st["dest"][0]] = len(values) - 1; st["val"] = len(values) - 1
    else: st["val"] = None
ALU = {"xor", "add", "rot", "move"}
other = {"a": "v", "v": "a"}
copies = {}  # (value, kind) -> name
def need(st, vi, k):
    """The statement reads value vi as kind k: a copy if the value is of the other kind."""
    if values[vi]["pool"] is None: values[vi]["pool"] = k; return
    if values[vi]["pool"] != k:
        copies.setdefault((vi, k), None); st.setdefault("use_copy", []).append((vi, k))
for st in stmts:
    svs = st["src_vals"]
    if st["kind"] in ALU:
        kinds = [values[sv]["pool"] for sv in svs]
        known = [k for k in kinds if k is not None]
        k = known[0] if known else ("a" if st["kind"] in ("rot", "move") else "v")  # no known operand: a default read kind
        for sv in svs: need(st, sv, k)
        if st["val"] is not None: values[st["val"]]["pool"] = other[k]
    else:  # hash unit
        if "--hash-va" in flags and st["kind"] == "hashrot":
            for sv in svs: need(st, sv, "v")
            if st["val"] is not None: values[st["val"]]["pool"] = "a"
        else:
            for sv in svs:
                if values[sv]["pool"] is None: values[sv]["pool"] = "v"
            if st["val"] is not None and values[st["val"]]["pool"] is None:
                values[st["val"]]["pool"] = "a" if st["kind"] == "hashrot" else "v"
for v in values:
    if v["pool"] is None: v["pool"] = "v"
# slots per kind by liveness
names = {"a": [], "v": []}; busy = {}
allocs = [(v["def"], v["last_read"], v["pool"], ("val", vi)) for vi, v in enumerate(values)]
allocs += [(values[vi]["def"] + 0.5, values[vi]["last_read"], k, ("copy", vi, k)) for (vi, k) in copies]
prefix = {"a": ("meta." if "--meta-a" in flags else "hdr.st."), "v": "hdr.st."}
for d, lr, p, who in sorted(allocs, key=lambda x: x[0]):
    chosen = None
    for n in names[p]:
        if busy[n] < d: chosen = n; break
    if chosen is None: chosen = f"{p}{len(names[p])}"; names[p].append(chosen)
    busy[chosen] = lr; name = prefix[p] + chosen
    if who[0] == "val": values[who[1]]["name"] = name
    else: copies[(who[1], who[2])] = name
def src_name(st, sv):
    for nd in st.get("use_copy", []):
        if nd[0] == sv: return copies[nd]
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
hdr_words = names["v"] + (names["a"] if "--meta-a" not in flags else [])
out = re.sub(r"header state_h \{.*?\n\}", "header state_h {\n" + "".join(f"  bit<32> {n};\n" for n in hdr_words) + "".join(f"  bit<32> s32_{i};\n" for i in range(11)) + "}", out, flags=re.S)  # the uncalled actions still name the old slots
meta_words = names["a"] if "--meta-a" in flags else []
out = re.sub(r"struct meta_t \{ .*? \}", "struct meta_t { " + " ".join(f"bit<32> {n};" for n in meta_words) + " bit<32> cond_operand_90_0_out; bit<32> op_xor_345_out; }", out)
if "--pragmas" in flags:
    prag = "".join(f'@pa_container_size("ingress", "{prefix[p]}{n}", 32)\n' for p in ("a", "v") for n in names[p])
    out = out.replace("control Ig(", prag + "control Ig(", 1)
old = "pkt.extract(hdr.recirc); transition accept; }"
assert out.count(old) == 1
out = out.replace(old, "pkt.extract(hdr.recirc); transition select(ig_intr_md.ingress_port) { 6: parse_st; default: accept; } }\n  state parse_st { pkt.extract(hdr.st); transition accept; }")
out = re.sub(r"action init\(\) \{[^}]*\}", "action init() { hdr.st.setValid(); " + " ".join(init_extra) + " }", out, count=1)
out = out.replace(t.splitlines()[0], "// " + comment, 1)
open(out_path, "w").write(out)
print(f"{out_path.split('/')[-1]}: a={len(names['a'])} v={len(names['v'])} copies={len(copies)} carried={sum(1 for v in values if v['kind']=='carried')} init-copies={len(init_extra)}")
