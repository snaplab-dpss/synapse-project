"""Per gress: the 32-bit fields an ALU statement of the chain ties together (slots, and the packet/register
fields read or written outside @in_hash next to a slot), which must fit one PHV group of twelve.
usage: cluster.py FILE.p4"""
import re, sys
src = open(sys.argv[1]).read()
eg = src.index("control Egress(")
for g, part in (("ingress", src[:eg]), ("egress", src[eg:])):
    slots, alu_inputs, hash_inputs, n_alu, n_hash = set(), set(), set(), 0, 0
    for m in re.finditer(r"  action ((?:compute|select)_\w+)\(\) \{(.*?)\n  \}", part, re.S):
        for line in m.group(2).splitlines():
            line = line.strip()
            if "hdr.st." not in line or "=" not in line: continue
            in_hash = line.startswith("@in_hash")
            fields = set(re.findall(r"hdr\.(?:hdr\d\.data\d+|egress_state\.\w+|recirc\.\w+)|(?:meta|eg_md)\.\w+", line))
            slots |= set(re.findall(r"hdr\.st\.(s\d+_\d+)", line))
            if in_hash: n_hash += 1; hash_inputs |= fields
            else: n_alu += 1; alu_inputs |= fields
    print(f"{g}: {len(slots)} slots + {len(alu_inputs)} fields tied by ALU statements = {len(slots) + len(alu_inputs)} (budget 12); ALU stmts {n_alu}, @in_hash stmts {n_hash}")
    print(f"   ALU-tied: {sorted(alu_inputs)}\n   hash-only: {sorted(hash_inputs - alu_inputs)}")
