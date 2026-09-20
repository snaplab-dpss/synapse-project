# CRC polynomial bank vs TNA `CRCPolynomial` (Tofino 2, SDE 9.13.4)

**Question.** Synapse's bloom filters, count-min sketches, HH-table sketches and cuckoo tables
index every row with the same CRC32 and a per-row salt, which makes the rows perfectly correlated
(`random_experiments/salted_crc_rows.py`). The fix is one CRC *polynomial* per row, emitted as
`Hash<bit<W>>(HashAlgorithm_t.CUSTOM, CRCPolynomial<bit<32>>(coeff, reversed, msb, extended,
init, xor))`, with libsycon and libnf computing bit-identical indices in software. Which software
CRC does each `CRCPolynomial` configuration compute, and does `Hash<bit<10>>` keep the low or the
high bits?

**Setup.** `gen_crcpoly.py` generates `crcpoly.p4` from the bank in `dpdk-nfs/lib/util/crc32.h`
(parsed by `random_experiments/crc32_polynomials.py`): the built-in `CRC32` and every bank entry
hash `{ipv4.src, tcp.dport}` (`msb = false`, `extended = false`); the 32-bit results and two
10-bit truncations are appended to the packet, which loops back to its ingress port.
`check_crcpoly.py` sends random packets and compares with a table-driven CRC-32 (normal-form
polynomial; `reversed` implemented as reflected input and output), built from the same bank.

```
python3 tofino/exp-hash/gen_crcpoly.py                        # after any change to the bank
make -f ../tools/Makefile install-tofino2 APP=crcpoly          # in this dir, in the SDE container
sudo -E env PATH=$PATH ./start_toy.sh crcpoly                  # model + bf_switchd, no controller
sudo -E env PATH=$PATH python3 tofino/exp-hash/check_crcpoly.py
sudo -E env PATH=$PATH python3 tests/testbed.py down; sudo pkill -x bf_switchd   # stop model, then switchd
```

**Takeaways (2026-09-20, 8 packets, ALL MATCH for the built-in CRC32 and all 16 bank entries).**

- The bank is the IEEE 802.3 CRC-32 plus 15 primitive polynomials (`0x7B17A39F`, `0x99F29AAD`,
  `0x21BCA2C3`, `0x0C596849`, `0x3553D717`, `0xAA1BC1D3`, `0x297D4A93`, `0xDE10F91F`,
  `0xDC077BC9`, `0xB5405445`, `0x391E2DDD`, `0x4E5ACD6D`, `0x6AEEFABD`, `0xFAF778B9`,
  `0x99DFB91B`), all `reversed = true`, `init = xor = 0xFFFFFFFF`. An earlier run of the same
  toy verified catalogue polynomials with other parameter mixes (CRC-32C/D/Q/AUTOSAR/CD-ROM-EDC/
  K/K2, including `reversed = false` with `init = xor = 0`) equally bit-exact, so the mapping
  holds for any coefficient/init/xor, reflected or not.
- `reversed = true` means reflected input bytes *and* reflected output (the usual "refin/refout"
  pair); `init` is the register's initial value and `xor` the final XOR, in that reflected domain.
- `Hash<bit<10>>` returns the **low** 10 bits of the 32-bit result, for reflected and
  non-reflected polynomials alike (`msb = false`), so software indexes with `crc & (width - 1)`.
- Seventeen custom hashes over a 6-byte input compile and place without complaint.
