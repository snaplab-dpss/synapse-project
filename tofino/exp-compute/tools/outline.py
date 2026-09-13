"""The apply block of a control as an outline: its branches, and each run of action calls as one
line with the calls that compute in the hash unit starred. usage: outline.py FILE.p4 CONTROL"""
import re, sys
path, ctrl = sys.argv[1:3]
t = open(path).read()
i = t.index(f"control {ctrl}(")
nxt = [m.start() for m in re.finditer(r"^control \w+\(", t, re.M) if m.start() > i]
body = t[i:nxt[0] if nxt else len(t)]
acts = {m.group(1): m.group(2) for m in re.finditer(r"  action (\w+)\(\)\s*\{(.*?)\}", body, re.S)}
apply = body[body.index("apply {"):]
depth = 0; seq = []
def flush():
    global seq
    if seq:
        names = [c + ("*" if "@in_hash" in acts.get(c, "") else "") for c in seq]
        print("  " * depth + f"[{len(seq)} calls, {sum(1 for c in seq if '@in_hash' in acts.get(c, ''))} hash] " + " ".join(names)[:150])
        seq = []
for line in apply.splitlines()[1:]:
    s = line.strip()
    if not s or s.startswith("//"): continue
    m = re.match(r"(\}\s*)?(else if|if|else)\b(.*)\{$", s)
    if m:
        flush()
        if m.group(1): depth = max(0, depth - 1)
        print("  " * depth + (m.group(2) + " " + m.group(3).strip())[:120]); depth += 1; continue
    if s == "}":
        flush(); depth = max(0, depth - 1); continue
    m = re.match(r"(\w+)\(\);", s)
    if m and m.group(1) in acts: seq.append(m.group(1)); continue
    m = re.match(r"(\w+)\.apply\(\)", s)
    if m: flush(); print("  " * depth + "table " + m.group(1)); continue
    flush(); print("  " * depth + "· " + s[:100])
flush()
