#!/usr/bin/env python3
"""
As a line-rate CACHE, is a real cuckoo table worse than the 2-way set-associative table our
Switcharoo port accidentally implemented?

Question: the P4 cuckoo table (synapse's template and the Switcharoo expert) files a key evicted
from table 1 into table 2 and, if that slot was live, recirculates to re-insert THAT key into
table 1, up to MAX_LOOPS times; the last displaced key is then dropped. With the salted hashes the
two tables were one bucket (h2 = h1 ^ const), so a displacement chain oscillated inside that
bucket; with two independent polynomials it is a real cuckoo random walk across the table. On the
hardware the real cuckoo raised the synapse KVS miss rate from 11.8% to 45%. Is that inherent to
cuckoo hashing in this regime, or a bug?

The regime is what makes it interesting, and it is not the one cuckoo hashing is designed for:
  - the table is a CACHE, so it is always ~100% occupied (never the <50% load factor cuckoo wants);
  - MAX_LOOPS is tiny (4), so most insertions fail rather than converge;
  - insertion is rare (only PUTs insert, 1% of traffic), so a hot key evicted by a failed chain is
    not restored until its next PUT -- losing a hot key is very expensive;
  - entries expire on a TTL.

Method: simulate both insert policies against the same zipf key stream and report the steady-state
miss rate. Everything else (table size, MAX_LOOPS, TTL, PUT ratio, skew) matches the hardware run.

How to run: python3 random_experiments/cuckoo_vs_2way_cache.py

Takeaways (2026-09-20): see the table printed at the end; the measured hardware miss rates were
11.8% (2-way) and 45.0% (real cuckoo) for kvs-f40000-c1000000-zipf1_2.
"""

import random

ENTRIES_PER_TABLE = 4096  # CUCKOO_ENTRIES
IDX_BITS = 12  # CUCKOO_IDX_WIDTH
IDX_MASK = (1 << IDX_BITS) - 1
MAX_LOOPS = 4  # CUCKOO_MAX_LOOPS
NUM_KEYS = 40_000  # TOTAL_FLOWS
ZIPF = 1.2  # ZIPF_PARAM
PUT_RATIO = 0.01  # 1 - KVS_GET_RATIO
PACKETS = 4_000_000
WARMUP = 2_000_000  # packets before we start counting

WAY_OFFSET = 0x893  # h2 = h1 ^ 0x893, the constant the two salts actually produced


class Cache:
    """Two tables of ENTRIES_PER_TABLE slots; a slot holds a key or None (expired/empty)."""

    def __init__(self, two_way: bool, rng: random.Random):
        self.two_way = two_way
        self.t1 = [None] * ENTRIES_PER_TABLE
        self.t2 = [None] * ENTRIES_PER_TABLE
        # Per-key hash values, drawn once (a hash is a fixed function of the key).
        self.h1 = [rng.getrandbits(IDX_BITS) for _ in range(NUM_KEYS)]
        if two_way:
            # The salted pair: table 2's index is table 1's XOR a constant.
            self.h2 = [h ^ (WAY_OFFSET & IDX_MASK) for h in self.h1]
        else:
            # Two independent polynomials.
            self.h2 = [rng.getrandbits(IDX_BITS) for _ in range(NUM_KEYS)]

    def lookup(self, key: int) -> bool:
        return self.t1[self.h1[key]] == key or self.t2[self.h2[key]] == key

    def insert(self, key: int) -> None:
        """One insertion, as the P4 does it: swap into table 1, push the evicted key into table 2,
        recirculate with table 2's evictee, up to MAX_LOOPS times, then drop whatever is in hand."""
        carried = key
        for _ in range(MAX_LOOPS):
            evicted_1 = self.t1[self.h1[carried]]
            self.t1[self.h1[carried]] = carried
            if evicted_1 is None:
                return
            # The evicted key goes to its own table-2 home (the real cuckoo) -- which for the 2-way
            # table is the same slot the incoming key would have used, since h2 = h1 ^ const.
            slot_2 = self.h2[evicted_1]
            evicted_2 = self.t2[slot_2]
            self.t2[slot_2] = evicted_1
            if evicted_2 is None:
                return
            carried = evicted_2  # recirculate and re-insert this one into table 1
        # MAX_LOOPS reached: `carried` is dropped, i.e. this key leaves the cache.


def zipf_stream(rng: random.Random, n: int, s: float, length: int) -> list[int]:
    """`length` key draws from a zipf(s) distribution over `n` keys, by inverse-CDF sampling."""
    weights = [1.0 / (i + 1) ** s for i in range(n)]
    total = 0.0
    cdf = []
    for w in weights:
        total += w
        cdf.append(total)
    cdf = [c / total for c in cdf]
    import bisect

    return [bisect.bisect_left(cdf, rng.random()) for _ in range(length)]


def run(two_way: bool, stream: list[int], rng: random.Random) -> float:
    cache = Cache(two_way, rng)
    misses = counted = 0
    for i, key in enumerate(stream):
        hit = cache.lookup(key)
        if i >= WARMUP:
            counted += 1
            misses += not hit
        # Only writes insert (the data plane recirculates a PUT that missed).
        if not hit and rng.random() < PUT_RATIO:
            cache.insert(key)
    return misses / counted


def main() -> None:
    rng = random.Random(20260920)
    print(f"{NUM_KEYS:,} keys, zipf {ZIPF}, 2 x {ENTRIES_PER_TABLE} entries, MAX_LOOPS {MAX_LOOPS}, "
          f"PUT ratio {PUT_RATIO}, {PACKETS:,} packets ({WARMUP:,} warmup)")
    stream = zipf_stream(rng, NUM_KEYS, ZIPF, PACKETS)
    distinct = len(set(stream))
    print(f"stream covers {distinct:,} distinct keys\n")
    print(f"{'design':38s} {'miss rate':>10s}   {'hardware':>10s}")
    for two_way, name, hw in ((True, "2-way set-associative (h2 = h1 ^ C)", "11.8%"),
                              (False, "real cuckoo (independent hashes)", "45.0%")):
        miss = run(two_way, stream, random.Random(7))
        print(f"{name:38s} {miss * 100:9.1f}%   {hw:>10s}")


if __name__ == "__main__":
    main()
