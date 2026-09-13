"""Match the state words the ingress parser extracts on every pass of a Tofino-model run (its
stdout.log) against a reference HalfSipHash trace of the test's flow, one line per word with the
reference state(s) it equals: which lane, which round, which step. A word that matches nothing is
where a pass went wrong. usage: siptrace.py MODEL_LOG SRC_IP SPORT DST_IP DPORT [SEQ] [KEY0] [KEY1]"""
import collections, re, sys

M32 = 0xFFFFFFFF


def rotl(x, n):
    return ((x << n) | (x >> (32 - n))) & M32


def reference(words, key0, key1):
    trace = []

    def sipround(v, tag):
        v[0] = (v[0] + v[1]) & M32; v[2] = (v[2] + v[3]) & M32; trace.append((tag + " a", list(v)))
        v[1] = rotl(v[1], 5); v[3] = rotl(v[3], 8); trace.append((tag + " b", list(v)))
        v[1] ^= v[0]; v[3] ^= v[2]; trace.append((tag + " c", list(v)))
        v[0] = rotl(v[0], 16); trace.append((tag + " d", list(v)))
        v[2] = (v[2] + v[1]) & M32; v[0] = (v[0] + v[3]) & M32; trace.append((tag + " e", list(v)))
        v[1] = rotl(v[1], 13); v[3] = rotl(v[3], 7); trace.append((tag + " f", list(v)))
        v[1] ^= v[2]; v[3] ^= v[0]; trace.append((tag + " g", list(v)))
        v[2] = rotl(v[2], 16); trace.append((tag + " h", list(v)))

    v = [key0 ^ 0x70736575, key1 ^ 0x6E646F6D, key0 ^ 0x6E657261, key1 ^ 0x79746573]
    trace.append(("init", list(v)))
    for i, m in enumerate(words):
        v[3] ^= m; trace.append((f"m{i} in", list(v)))
        sipround(v, f"m{i} r1"); sipround(v, f"m{i} r2")
        v[0] ^= m; trace.append((f"m{i} out", list(v)))
    for k in range(4):
        sipround(v, f"fin r{k + 1}")
    return trace, v[0] ^ v[1] ^ v[2] ^ v[3]


ip = lambda a: int.from_bytes(bytes(int(b) for b in a.split(".")), "big")
log, src, sport, dst, dport = sys.argv[1], ip(sys.argv[2]), int(sys.argv[3]), ip(sys.argv[4]), int(sys.argv[5])
seq = int(sys.argv[6], 0) if len(sys.argv) > 6 else 0x11223344
key0 = int(sys.argv[7], 0) if len(sys.argv) > 7 else 0x33323130
key1 = int(sys.argv[8], 0) if len(sys.argv) > 8 else 0x42413938
trace, h = reference([src, dst, (sport << 16) | dport, seq], key0, key1)
print(f"reference hash {h:08x}; final v {[f'{x:08x}' for x in trace[-1][1]]}")
lookup = {}
for tag, v in trace:
    for i, x in enumerate(v):
        lookup.setdefault(x, []).append(f"{tag}/v{i}")
passes = collections.OrderedDict()
for l in open(log):
    if "Parser state" not in l:
        continue
    m = re.search(r":(0x[0-9a-f]+):-:.*to value +(0x[0-9a-f]+) I \[((?:hdr\.st\.s32|hdr\.recirc\.f32|hdr\.cpu\.)[^\[\]]*)", l)
    if m:
        passes.setdefault(m.group(1), collections.OrderedDict())[m.group(3)] = int(m.group(2), 16)
for p, fields in passes.items():
    print(f"== pass {p}")
    for f, v in fields.items():
        print(f"   {f:22s} {v:08x}  {lookup.get(v, '')}")
