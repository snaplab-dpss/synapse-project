#!/usr/bin/env python3
"""
Send TCP packets through crcpoly.p4 on the tofino-model and compare the hash outputs it appends
(out_h: the built-in CRC32 and every CRC32_BANK entry) with software CRCs of the same input bytes
{ipv4.src, tcp.dport}.

Run inside the SDE container, as root, with the model and switchd up (see README.md):
  sudo -E env PATH=$PATH python3 tofino/exp-hash/check_crcpoly.py
"""

import importlib.util
import random
import struct
import sys
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent.parent / "tests"))

from util import Ports, TCP, IP, Ether  # noqa: E402

PORT = 3  # front panel port; the program loops the packet back to its ingress port


def make_crc32(poly: int, reflected: bool, init: int, xor: int):
    """Table-driven CRC-32 of the given polynomial (normal form), reflecting input and output when
    `reflected`; the same construction libsycon's CRC32 uses for the IEEE polynomial."""
    if reflected:
        rpoly = int(f"{poly:032b}"[::-1], 2)
        table = []
        for n in range(256):
            c = n
            for _ in range(8):
                c = (c >> 1) ^ rpoly if c & 1 else c >> 1
            table.append(c)

        def crc(data: bytes) -> int:
            c = init
            for b in data:
                c = table[(c ^ b) & 0xFF] ^ (c >> 8)
            return (c ^ xor) & 0xFFFFFFFF

    else:
        table = []
        for n in range(256):
            c = n << 24
            for _ in range(8):
                c = ((c << 1) ^ poly) & 0xFFFFFFFF if c & 0x80000000 else (c << 1) & 0xFFFFFFFF
            table.append(c)

        def crc(data: bytes) -> int:
            c = init
            for b in data:
                c = table[((c >> 24) ^ b) & 0xFF] ^ ((c << 8) & 0xFFFFFFFF)
            return (c ^ xor) & 0xFFFFFFFF

    return crc


# The bank crcpoly.p4 was generated from (gen_crcpoly.py): (name, coeff, reversed, init, xor).
spec = importlib.util.spec_from_file_location("crc32_polynomials", Path(__file__).resolve().parent.parent.parent / "random_experiments" / "crc32_polynomials.py")
polynomials = importlib.util.module_from_spec(spec)
spec.loader.exec_module(polynomials)
BANK = polynomials.read_bank()
CRCS = {name: make_crc32(*cfg) for name, *cfg in BANK}
assert CRCS["ieee"](b"123456789") == zlib.crc32(b"123456789") == 0xCBF43926


def main() -> None:
    rng = random.Random(7)
    ports = Ports([PORT], verbose=False)
    n = len(BANK)
    out_size = 4 * (n + 1) + 2 + 2
    mismatches = 0
    for k in range(8):
        src = rng.getrandbits(32)
        dport = rng.getrandbits(16)
        pkt = Ether(dst="00:11:22:33:44:55", src="66:77:88:99:aa:bb") / IP(src=".".join(str(b) for b in src.to_bytes(4, "big")), dst="10.0.0.1") / TCP(sport=1234, dport=dport, seq=0, flags="S")
        ports.drain()
        ports.send(PORT, pkt)
        got = ports.collect(2.0)
        if not got:
            print(f"packet {k}: nothing came back")
            mismatches += 1
            continue
        # The program's tcp_h is 12 bytes (scapy's TCP is 20), so out_h starts at eth(14)+ipv4(20)+12.
        out = got[0].raw[14 + 20 + 12 : 14 + 20 + 12 + out_size]
        h = struct.unpack(f">{n + 1}IHH", out)
        key = struct.pack(">IH", src, dport)
        expected = [zlib.crc32(key)] + [CRCS[name](key) for name, *_ in BANK]
        bad = []
        for i, name in enumerate(["builtin"] + [b[0] for b in BANK]):
            if h[i] != expected[i]:
                bad.append(f"{name}={h[i]:08x}!={expected[i]:08x}")
        mismatches += len(bad)
        t0, t1 = h[n + 1], h[n + 2]
        e0, e1 = expected[1], expected[2]
        trunc = []
        for t, e, name in ((t0, e0, BANK[0][0]), (t1, e1, BANK[1][0])):
            trunc.append(f"{name}10={'low' if t == e & 0x3FF else 'HIGH' if t == e >> 22 else f'?{t:03x}'}")
        print(f"packet {k} src={src:08x} dport={dport:04x}: {n + 1} hashes {'all ok' if not bad else ' '.join(bad)} " + " ".join(trunc))
    print("ALL MATCH" if mismatches == 0 else f"{mismatches} mismatches")


if __name__ == "__main__":
    main()
