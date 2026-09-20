#!/usr/bin/env python3
"""
Which degree-32 polynomials should the CRC32_BANK (dpdk-nfs/lib/util/crc32.h) hold?

Question: the bank gives each row of a bloom filter / count-min sketch its own CRC-32 polynomial,
so the rows collide on different key pairs. Two rows are as independent as their polynomials are
coprime: if p = f*g and q = f*h share a factor f, every key pair whose difference f divides
collides in both rows. Irreducible polynomials are pairwise coprime by construction, and primitive
ones (order 2^32 - 1) are the classic CRC choice. Are the catalogue polynomials irreducible, and
how do we get a bank of 16 that provably are?

Method: arithmetic over GF(2)[x]; irreducibility by Rabin's test (x^(2^32) == x mod p and
gcd(x^(2^(32/r)) - x, p) == 1 for the prime factors r of 32, i.e. r = 2), primitivity by checking
that x has order exactly 2^32 - 1 = 3 * 5 * 17 * 257 * 65537 modulo p. Then search from a fixed
seed for primitive polynomials to fill the bank, and print it as the C initializer.

How to run: python3 random_experiments/crc32_polynomials.py

Takeaways (2026-09-20):
  - only 3 of the 9 catalogue CRC-32s are irreducible (ieee and xfer primitive, d irreducible);
    c, q, autosar, cdrom, k and k2 all carry the factor (x+1) (they were designed for error
    detection, where that helps), and autosar/cdrom/k2 share a degree-2 factor -- as hash rows
    those pairs would be mildly correlated again;
  - so the bank is ieee (primitive) + 15 generated primitive polynomials (seed 0x5EED, first 15 of
    the search below), all reflected with init = xor = 0xFFFFFFFF like ieee. `check_bank()` parses
    the C header AND synapse's C++ copy (kept separate on purpose), and re-verifies that they are
    identical, that every entry is primitive and distinct, and that entry 0 is ieee.
"""

import random

DEGREE = 32
ORDER = (1 << DEGREE) - 1
ORDER_PRIME_FACTORS = [3, 5, 17, 257, 65537]
assert 3 * 5 * 17 * 257 * 65537 == ORDER

CATALOGUE = [
    ("ieee", 0x04C11DB7),
    ("c", 0x1EDC6F41),
    ("d", 0xA833982B),
    ("q", 0x814141AB),
    ("autosar", 0xF4ACFB13),
    ("cdrom", 0x8001801B),
    ("k", 0x741B8CD7),
    ("k2", 0x32583499),
    ("xfer", 0x000000AF),
]


def full(poly: int) -> int:
    """Normal-form 32-bit coefficients -> the full polynomial with the implied x^32 term."""
    return poly | (1 << DEGREE)


def mulmod(a: int, b: int, m: int) -> int:
    r = 0
    while b:
        if b & 1:
            r ^= a
        b >>= 1
        a <<= 1
        if a >> DEGREE & 1:
            a ^= m
    return r


def powmod(a: int, e: int, m: int) -> int:
    r = 1
    while e:
        if e & 1:
            r = mulmod(r, a, m)
        a = mulmod(a, a, m)
        e >>= 1
    return r


def gcd(a: int, b: int) -> int:
    while b:
        while a.bit_length() >= b.bit_length() and a:
            a ^= b << (a.bit_length() - b.bit_length())
        a, b = b, a
    return a


def is_irreducible(poly: int) -> bool:
    m = full(poly)
    x = 2
    if powmod(x, 1 << DEGREE, m) != x:
        return False
    # Rabin: for each prime r dividing the degree, gcd(x^(2^(n/r)) - x, p) must be 1.
    y = powmod(x, 1 << (DEGREE // 2), m) ^ x
    return gcd(m, y) == 1


def is_primitive(poly: int) -> bool:
    if not is_irreducible(poly):
        return False
    m = full(poly)
    x = 2
    if powmod(x, ORDER, m) != 1:
        return False
    return all(powmod(x, ORDER // r, m) != 1 for r in ORDER_PRIME_FACTORS)


def classify(poly: int) -> str:
    if is_primitive(poly):
        return "primitive"
    if is_irreducible(poly):
        return "irreducible"
    return "reducible"


LIBNF_HEADER = "dpdk-nfs/lib/util/crc32.h"
SYNAPSE_HEADER = "synapse/libraries/LibSynapse/Modules/Tofino/DataStructures/Hash.h"


def read_bank(header: str = LIBNF_HEADER) -> list[tuple[str, int, bool, int, int]]:
    """The CRC32_BANK entries of a header: (name, coeff, reversed, init, xor_out). libnf (C) and
    synapse (C++) each keep their own copy, deliberately not sharing code; this reads either."""
    import re
    from pathlib import Path

    text = (Path(__file__).resolve().parent.parent / header).read_text()
    body = text[text.index("CRC32_BANK") :]
    body = body[body.index("{") : body.index("};")]
    bank = []
    for m in re.finditer(r'\{"(\w+)",\s*(0x[0-9A-Fa-f]+),\s*(true|false),\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+)\}', body):
        bank.append((m.group(1), int(m.group(2), 16), m.group(3) == "true", int(m.group(4), 16), int(m.group(5), 16)))
    size = int(re.search(r"CRC32_BANK_SIZE\s*=?\s*(\d+)", text).group(1))
    assert len(bank) == size, f"{header}: parsed {len(bank)} entries, CRC32_BANK_SIZE is {size}"
    return bank


def check_bank() -> None:
    bank = read_bank(LIBNF_HEADER)
    assert bank == read_bank(SYNAPSE_HEADER), "libnf's and synapse's CRC32_BANK differ"
    assert bank[0][:2] == ("ieee", 0x04C11DB7), "entry 0 must stay the IEEE CRC-32"
    coeffs = [c for _, c, *_ in bank]
    assert len(set(coeffs)) == len(coeffs), "duplicate polynomial in the bank"
    print(f"CRC32_BANK ({len(bank)} entries):")
    for name, coeff, reversed_, init, xor in bank:
        kind = classify(coeff)
        assert kind == "primitive", f"{name} 0x{coeff:08X} is {kind}"
        print(f"  {name:5s} 0x{coeff:08X} reversed={reversed_} init=0x{init:08X} xor=0x{xor:08X} {kind}")
    print("  all primitive, all distinct, libnf and synapse copies identical")


def main() -> None:
    check_bank()

    print("\ncatalogue polynomials:")
    for name, poly in CATALOGUE:
        print(f"  {name:8s} 0x{poly:08X} {classify(poly)}")

    # A deterministic search for primitive polynomials to fill the bank with, skipping the catalogue
    # ones so the printed table can keep them by name.
    rng = random.Random(0x5EED)
    catalogue_polys = {p for _, p in CATALOGUE}
    generated = []
    while len(generated) < 16:
        poly = rng.getrandbits(DEGREE) | 1  # a CRC polynomial has the x^0 term
        if poly in catalogue_polys or poly in generated:
            continue
        if is_primitive(poly):
            generated.append(poly)
    print("\ngenerated primitive polynomials (seed 0x5EED):")
    for poly in generated:
        print(f"  0x{poly:08X}")


if __name__ == "__main__":
    main()
