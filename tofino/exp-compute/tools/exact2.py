"""Global exact search: every path in dump order, both gresses, one word per op over all paths;
budgets per gress (words a gress's values write or read). usage: exact2.py DUMP KI KE"""
import re, sys, time
sys.argv, args = sys.argv[:2], sys.argv[2:]
import os
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "realloc.py")).read().split("for name in (sys.argv[2:]")[0]
exec(src)
KI, KE = int(args[0]), int(args[1]); K = max(KI, KE) + 4
# distinct ops across paths, with per-path live ranges; sources per op (same on every path)
ops = {}
for pi, vals in enumerate(all_paths):
    for v in vals:
        o = ops.setdefault(v["op"], dict(op=v["op"], eg=v["eg"], srcs=set(), ranges=[], first=(pi, v["fr"])))
        o["ranges"].append((pi, v["fr"], v["to"])); o["srcs"].update(v["src_ops"])
order = sorted(ops.values(), key=lambda o: o["first"])
idx = {o["op"]: i for i, o in enumerate(order)}; n = len(order)
srcs = [[(idx[s], r) for s, r in o["srcs"] if s in idx] for o in order]
readers = [[] for _ in order]
for i, ss in enumerate(srcs):
    for j, r in ss: readers[j].append((i, r))
half = {i: idx[o["op"][:-3] + "_shl"] for i, o in enumerate(order) if o["op"].endswith("_or") and o["op"][:-3] + "_shl" in idx}
assign = [-1] * n
occupancy = [[] for _ in range(K)]   # word -> list of (path, from, to, op index)
rot = {}; rot_cnt = {}
used = {False: [0] * K, True: [0] * K}  # per gress: how many assigned values touch the word
def touches(i, w):  # words this op would make its gress touch: its own word and its sources' words
    return None
def busy_conflict(i, w):
    o = order[i]
    for pi, fr, to in o["ranges"]:
        for (pj, f2, t2, k) in occupancy[w]:
            if pj != pi: continue
            if t2 < fr or to < f2: continue
            if t2 == fr and i in half and half[i] == k: continue  # the or over its shl half
            return True
    return False
def feasible(i, w):
    if busy_conflict(i, w): return False
    for j, r in srcs[i]:
        if assign[j] >= 0 and rot.get((order[i]["eg"], w, assign[j]), r) != r: return False
    for j, r in readers[i]:
        if assign[j] >= 0 and rot.get((order[j]["eg"], assign[j], w), r) != r: return False
    # budgets: words touched by this op's gress
    eg = order[i]["eg"]; budget = KE if eg else KI
    touched = {w} | {assign[j] for j, r in srcs[i] if assign[j] >= 0}
    cur = {x for x in range(K) if used[eg][x] > 0}
    if len(cur | touched) > budget: return False
    return True
def place(i, w):
    o = order[i]; added = []
    for pi, fr, to in o["ranges"]: occupancy[w].append((pi, fr, to, i))
    eg = o["eg"]; used[eg][w] += 1
    for j, r in srcs[i]:
        if assign[j] >= 0:
            used[eg][assign[j]] += 1
            k = (eg, w, assign[j]); rot_cnt[k] = rot_cnt.get(k, 0) + 1
            if k not in rot: rot[k] = r; added.append(k)
    for j, r in readers[i]:
        if assign[j] >= 0:
            used[order[j]["eg"]][w] += 1
            k = (order[j]["eg"], assign[j], w); rot_cnt[k] = rot_cnt.get(k, 0) + 1
            if k not in rot: rot[k] = r; added.append(k)
    return added
def unplace(i, w, added):
    o = order[i]
    occupancy[w] = [x for x in occupancy[w] if x[3] != i]
    eg = o["eg"]; used[eg][w] -= 1
    for j, r in srcs[i]:
        if assign[j] >= 0: used[eg][assign[j]] -= 1
    for j, r in readers[i]:
        if assign[j] >= 0: used[order[j]["eg"]][w] -= 1
    for k in added: del rot[k]
nodes = 0; t0 = time.time(); LIMIT = 2_000_000
def dfs(i):
    global nodes
    if i == n: return True
    nodes += 1
    if nodes > LIMIT or time.time() - t0 > 240: return False
    fresh_tried = False
    for w in range(K):
        fresh = not occupancy[w]
        if fresh and fresh_tried: continue
        if fresh: fresh_tried = True
        if not feasible(i, w): continue
        assign[i] = w; added = place(i, w)
        if dfs(i + 1): return True
        unplace(i, w, added); assign[i] = -1
    return False
ok = dfs(0)
wi = sum(1 for x in range(K) if used[False][x] > 0); we = sum(1 for x in range(K) if used[True][x] > 0)
print(f"budgets ingress {KI} egress {KE}: {'FEASIBLE' if ok else 'not found'} after {nodes} nodes, {time.time()-t0:.1f}s; words ingress {wi} egress {we}, total {len(set(assign)) if ok else '-'}")
