#!/usr/bin/env python3
"""
Are the rows of a bloom filter / count-min sketch independent when every row hashes the key with
the same CRC and only a per-row salt appended to the input?

Question: the synthesized bloom filters and count-min sketches (and libsycon, which mirrors them)
index row i with CRC32(key ++ SALT_i) truncated to log2(width) bits. The model test of
psd-f40000-c1000-zipf0_6 showed two different (src, dst port) keys landing on the same cell in
all four rows at once, seven ports in a row. Does salting a CRC give independent rows?

Answer: no. CRC is affine over GF(2): for messages of equal length,
    crc(a) ^ crc(b) = crc(a ^ b) ^ crc(0),
so
    crc(x ++ s) == crc(y ++ s)   <=>   crc((x ^ y) ++ 0) == crc(0),
which does not depend on the salt s. Two keys that collide in one row collide in every row, and
truncating to the low bits keeps the equality. Salting only permutes each row's cells; it does not
decorrelate them. A k-row filter built this way has the false-positive rate of one row, and the
"min over rows" of the sketch is one row's counter. Rows are independent only with different hash
*functions*: CRCs with different polynomials (Tofino: HashAlgorithm_t.CUSTOM + CRCPolynomial), or
any non-linear hash.

How to run: python3 random_experiments/salted_crc_rows.py   (stdlib only, ~10 s)

Takeaways (seed 1; 400k pairs, 200k queries):
  scheme                               P(all rows | row 0)   bloom FP (50 keys, 4x1024)
  CRC32 + appended salt (synapse)      389/389 = 1.000       4.9e-2  (= single row: 1-e^(-50/1024) = 4.8e-2)
  CRC32 with a different poly per row  0/389   = 0.000       0       (expected (1-e^(-50/1024))^4 = 5e-6)
  blake2b keyed per row (control)      0/402   = 0.000       5.0e-6
  - the affine identity holds for every sampled pair (crc32 and crc32c, init/xorout included);
  - the four row indices the tofino-model logged for (0x8F505DE5, 5002) and (0x4FC36ECA, 8001)
    are reproduced by zlib.crc32 and are equal, as the identity predicts;
  - ports 8000+i and 5000+j alias whenever i ^ j == 3 (the bases differ in bits above the low 3),
    so a fresh source whose Δsrc collides with one earlier source gets a whole block of false
    positives: 16 of the 24 scanned ports (8000-8007 and 8016-8023) in the failing run.
"""

import hashlib
import math
import random
import struct
import zlib

WIDTH_BITS = 10
WIDTH = 1 << WIDTH_BITS
ROWS = 4

# The salts synapse's TofinoSynthesizer emits and libsycon's BloomFilter::HASH_SALTS use.
SALTS = [0xFBC31FC7, 0x2681580B, 0x486D7E2F, 0x1F3A2B4D]


# --- generic table-driven CRC32 so we can change the polynomial per row -------------------------


def make_crc32(poly_reflected: int):
    table = []
    for n in range(256):
        c = n
        for _ in range(8):
            c = (c >> 1) ^ poly_reflected if c & 1 else c >> 1
        table.append(c)

    def crc(data: bytes, init: int = 0xFFFFFFFF, xorout: int = 0xFFFFFFFF) -> int:
        c = init
        for b in data:
            c = table[(c ^ b) & 0xFF] ^ (c >> 8)
        return c ^ xorout

    return crc


CRC32 = make_crc32(0xEDB88320)  # IEEE 802.3, what zlib.crc32 and Tofino's CRC32 compute
CRC32C = make_crc32(0x82F63B78)  # Castagnoli
CRC32K = make_crc32(0xEB31D82E)  # Koopman
CRC32Q = make_crc32(0xD5828281)  # CRC-32Q

assert CRC32(b"123456789") == zlib.crc32(b"123456789") == 0xCBF43926
assert CRC32C(b"123456789") == 0xE3069283


# --- the three row-hashing schemes ------------------------------------------------------------


def key_bytes(src: int, port: int) -> bytes:
    return struct.pack(">IH", src, port)


def rows_salted_crc(key: bytes) -> list[int]:
    """What synapse emits: one CRC32, the row's salt appended to the input."""
    return [zlib.crc32(key + struct.pack(">I", s)) & (WIDTH - 1) for s in SALTS]


def rows_poly_crc(key: bytes) -> list[int]:
    """A different CRC polynomial per row (no salt needed)."""
    return [f(key) & (WIDTH - 1) for f in (CRC32, CRC32C, CRC32K, CRC32Q)]


def rows_keyed_blake(key: bytes) -> list[int]:
    """Control: a non-linear hash keyed per row."""
    out = []
    for s in SALTS:
        h = hashlib.blake2b(key, key=struct.pack(">I", s), digest_size=4).digest()
        out.append(int.from_bytes(h, "big") & (WIDTH - 1))
    return out


SCHEMES = {
    "CRC32 + appended salt (synapse)": rows_salted_crc,
    "CRC32 with a different poly per row": rows_poly_crc,
    "blake2b keyed per row (control)": rows_keyed_blake,
}


# --- experiments ---------------------------------------------------------------------------------


def random_key(rng: random.Random) -> bytes:
    return key_bytes(rng.getrandbits(32), rng.getrandbits(16))


def affine_identity_holds(rng: random.Random, samples: int) -> bool:
    """crc(a) ^ crc(b) == crc(a ^ b) ^ crc(0) for equal-length a, b (init and xorout included)."""
    for crc in (CRC32, CRC32C):
        for _ in range(samples):
            n = rng.randint(1, 16)
            a = bytes(rng.getrandbits(8) for _ in range(n))
            b = bytes(rng.getrandbits(8) for _ in range(n))
            ab = bytes(x ^ y for x, y in zip(a, b))
            if crc(a) ^ crc(b) != crc(ab) ^ crc(bytes(n)):
                return False
    return True


def all_rows_given_row0(rows, rng: random.Random, pairs: int) -> tuple[int, int]:
    """Count random key pairs colliding in row 0, and how many of those collide in every row."""
    row0 = all_rows = 0
    for _ in range(pairs):
        a, b = rows(random_key(rng)), rows(random_key(rng))
        if a[0] == b[0]:
            row0 += 1
            all_rows += a == b
    return row0, all_rows


def bloom_false_positives(rows, rng: random.Random, inserted: int, queries: int) -> float:
    bits = [[False] * WIDTH for _ in range(ROWS)]
    for _ in range(inserted):
        for r, i in enumerate(rows(random_key(rng))):
            bits[r][i] = True
    fp = 0
    for _ in range(queries):
        fp += all(bits[r][i] for r, i in enumerate(rows(random_key(rng))))
    return fp / queries


def psd_alias_pattern() -> list[int]:
    """Ports 8000+i (fresh source) that alias a port 5000+j of an earlier source, for the two
    sources of the failing model run (the four row indices the tofino-model logged for
    (0x8F505DE5, 5002) and (0x4FC36ECA, 8001) were {0x1f, 0x3ac, 0x3e6, 0x1ad}, in both)."""
    earlier, fresh = 0x8F505DE5, 0x4FC36ECA
    seen = {tuple(rows_salted_crc(key_bytes(earlier, 5000 + j))) for j in range(24)}
    return [8000 + i for i in range(24) if tuple(rows_salted_crc(key_bytes(fresh, 8000 + i))) in seen]


def main() -> None:
    rng = random.Random(1)

    print(f"affine identity crc(a)^crc(b) == crc(a^b)^crc(0): {affine_identity_holds(rng, 20_000)}")

    logged = rows_salted_crc(key_bytes(0x8F505DE5, 5002)), rows_salted_crc(key_bytes(0x4FC36ECA, 8001))
    print(f"model-run pair: {[hex(i) for i in logged[0]]} vs {[hex(i) for i in logged[1]]} -> equal={logged[0] == logged[1]}")
    print(f"fresh-source ports aliasing an earlier source's port: {psd_alias_pattern()}")
    print()

    inserted, queries, pairs = 50, 200_000, 400_000
    one_row = 1 - math.exp(-inserted / WIDTH)
    print(f"{'scheme':40s} {'row0 coll.':>10s} {'all rows':>9s} {'P(all|row0)':>12s} {'bloom FP':>10s}")
    for name, rows in SCHEMES.items():
        row0, both = all_rows_given_row0(rows, rng, pairs)
        fp = bloom_false_positives(rows, rng, inserted, queries)
        print(f"{name:40s} {row0:10d} {both:9d} {both / row0:12.3f} {fp:10.2e}")
    print(f"{'expected, 1 row':40s} {'':10s} {'':9s} {'':12s} {one_row:10.2e}")
    print(f"{'expected, 4 independent rows':40s} {'':10s} {'':9s} {'':12s} {one_row ** ROWS:10.2e}")


if __name__ == "__main__":
    main()
