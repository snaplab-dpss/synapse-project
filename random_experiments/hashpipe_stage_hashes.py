#!/usr/bin/env python3
"""
Do the CRC32_BANK polynomials (lib/util/math.h) give independent per-stage indices for a
HashPipe over /24 prefixes?

Question: the HashPipe HHH (dpdk-nfs/hhh) indexes stage i of its /24 table with
crc32_hasher_hash(hashers[i], prefix, 4) & (WIDTH - 1), hasher i
initialized with CRC32_BANK[i]. The keys are structured (4-byte
little-endian masked addresses, low byte zero) rather than random, and the paper's eviction
relies on the d stages sampling d *different* slots per key. salted_crc_rows.py showed that
salting one CRC makes every stage collide on the same pairs; are different polynomials enough on
these keys, and is slicing one CRC32 into d index fields (d * log2(WIDTH) <= 32) a sound cheaper
alternative when it fits?

How to run: python3 random_experiments/hashpipe_stage_hashes.py   (stdlib only, ~5 s)

Takeaways (seed 1, 40k keys, 6 stages x 1024 slots):
  scheme                 uniform /24s                 clustered /24s
  bank polynomials       P(all|0) 0  P(>=2) 0.0046    P(all|0) 0  P(>=2) 0.0064   (ideal 0.0049)
  salted CRC32           P(all|0) 1  P(>=2) 1.0000    P(all|0) 1  P(>=2) 1.0000   (the trap)
  one CRC32 sliced, d=3  P(all|0) 0  P(>=2) 0.0017    P(all|0) 0  P(>=2) 0.0014   (ideal 0.0020)
  - the six bank polynomials index the stages as independent hash functions would, on structured
    keys too (clustered: 0.64% vs 0.49% ideal, within the noise of 790k pairs at this width);
  - salting reproduces salted_crc_rows.py: every pair colliding in stage 0 collides everywhere;
  - slicing one CRC32 is sound (different linear projections, no pair collides in all three
    slices) but only covers d * log2(width) <= 32 bits, i.e. not d = 6 at 1024 slots.

Metrics, per scheme and key distribution (N keys into d stages of WIDTH slots):
  load      occupied slots per stage vs. the ideal WIDTH * (1 - e^(-N/WIDTH))
  P(all|0)  among key pairs colliding in stage 0, the fraction colliding in EVERY stage
            (independent stages: (1/WIDTH)^(d-1); salting: 1)
  P(>=2)    fraction of colliding-in-stage-0 pairs that also collide in at least one other stage
            (independent: 1 - (1 - 1/WIDTH)^(d-1))
"""

import math
import random
import zlib

WIDTH_BITS = 10
WIDTH = 1 << WIDTH_BITS
STAGES = 6
N_KEYS = 40_000
SEED = 1

# CRC32_BANK from lib/util/math.h, normal (non-reflected) form; entry 0 is IEEE 802.3.
CRC32_BANK = [
    0x04C11DB7, 0x7B17A39F, 0x99F29AAD, 0x21BCA2C3, 0x0C596849, 0x3553D717, 0xAA1BC1D3, 0x297D4A93,
    0xDE10F91F, 0xDC077BC9, 0xB5405445, 0x391E2DDD, 0x4E5ACD6D, 0x6AEEFABD, 0xFAF778B9, 0x99DFB91B,
]


def reflect32(value: int) -> int:
    return int(f"{value:032b}"[::-1], 2)


def make_crc32(poly_normal: int):
    """Reflected table-driven CRC-32 with init = xorout = 0xFFFFFFFF, as crc32_hasher computes it."""
    poly = reflect32(poly_normal)
    table = []
    for n in range(256):
        c = n
        for _ in range(8):
            c = (c >> 1) ^ poly if c & 1 else c >> 1
        table.append(c)

    def crc(data: bytes) -> int:
        c = 0xFFFFFFFF
        for b in data:
            c = table[(c ^ b) & 0xFF] ^ (c >> 8)
        return c ^ 0xFFFFFFFF

    return crc


CRCS = [make_crc32(p) for p in CRC32_BANK]
assert CRCS[0](b"123456789") == zlib.crc32(b"123456789") == 0xCBF43926


def key_bytes(prefix24: int) -> bytes:
    # The NF's key: the IPv4 source as stored in the packet (big-endian bytes), masked to /24,
    # read as a 4-byte object. Byte 3 is the masked-off host byte.
    return bytes([(prefix24 >> 16) & 0xFF, (prefix24 >> 8) & 0xFF, prefix24 & 0xFF, 0])


# --- key distributions ---------------------------------------------------------------------------


def uniform_prefixes(rng: random.Random, n: int) -> list[int]:
    return rng.sample(range(1 << 24), n)


def clustered_prefixes(rng: random.Random, n: int) -> list[int]:
    """Prefixes concentrated in a few /16 blocks, as real sources are: 90% of the keys in 256 /16
    blocks (a quarter of their /24s), the rest anywhere."""
    blocks = rng.sample(range(1 << 16), 256)
    keys = set()
    while len(keys) < int(0.9 * n):
        keys.add((rng.choice(blocks) << 8) | rng.randrange(256))
    while len(keys) < n:
        keys.add(rng.randrange(1 << 24))
    return list(keys)


# --- index schemes: key -> [stage index] ---------------------------------------------------------


def scheme_bank(stages: int):
    return lambda k: [CRCS[i](key_bytes(k)) & (WIDTH - 1) for i in range(stages)]


def scheme_salted(stages: int):
    # One CRC, a per-stage salt appended: the synapse-of-old row scheme (see salted_crc_rows.py).
    salts = [0xFBC31FC7, 0x2681580B, 0x486D7E2F, 0x1F3A2B4D, 0x5A3C9E71, 0x9D2F4B63]
    return lambda k: [CRCS[0](key_bytes(k) + salts[i].to_bytes(4, "little")) & (WIDTH - 1) for i in range(stages)]


def scheme_sliced(stages: int):
    # One CRC32, stage i indexed by bits [WIDTH_BITS*i + WIDTH_BITS - 1 : WIDTH_BITS*i].
    assert stages * WIDTH_BITS <= 32
    return lambda k: [(CRCS[0](key_bytes(k)) >> (WIDTH_BITS * i)) & (WIDTH - 1) for i in range(stages)]


# --- measurement -----------------------------------------------------------------------------------


def measure(keys: list[int], index_fn, stages: int) -> str:
    idx = {k: index_fn(k) for k in keys}
    n = len(keys)

    loads = [len({idx[k][s] for k in keys}) for s in range(stages)]
    ideal_load = WIDTH * (1 - math.exp(-n / WIDTH))

    by_slot0: dict[int, list[int]] = {}
    for k in keys:
        by_slot0.setdefault(idx[k][0], []).append(k)
    pairs = all_pairs = any_other = 0
    for group in by_slot0.values():
        for i in range(len(group)):
            for j in range(i + 1, len(group)):
                a, b = idx[group[i]], idx[group[j]]
                pairs += 1
                others = [a[s] == b[s] for s in range(1, stages)]
                all_pairs += all(others)
                any_other += any(others)
    p_all = all_pairs / pairs if pairs else float("nan")
    p_any = any_other / pairs if pairs else float("nan")
    ideal_all = (1 / WIDTH) ** (stages - 1)
    ideal_any = 1 - (1 - 1 / WIDTH) ** (stages - 1)
    return (f"load {min(loads)}-{max(loads)} (ideal {ideal_load:.0f})  "
            f"P(all|0) {p_all:.2e} (ideal {ideal_all:.0e})  "
            f"P(>=2) {p_any:.4f} (ideal {ideal_any:.4f})  [{pairs} pairs]")


def main():
    rng = random.Random(SEED)
    for name, keys in [("uniform /24s", uniform_prefixes(rng, N_KEYS)), ("clustered /24s", clustered_prefixes(rng, N_KEYS))]:
        print(f"== {name}: {len(keys)} keys, {STAGES} stages x {WIDTH} slots")
        print(f"   bank polynomials  {measure(keys, scheme_bank(STAGES), STAGES)}")
        print(f"   salted CRC32      {measure(keys, scheme_salted(STAGES), STAGES)}")
        print(f"   one CRC32 sliced  {measure(keys, scheme_sliced(3), 3)}   (d = 3 only: 3 x {WIDTH_BITS} bits)")


if __name__ == "__main__":
    main()
