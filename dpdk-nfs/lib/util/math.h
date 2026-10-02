#ifndef MATH_H_INCLUDED
#define MATH_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

unsigned hash_obj(void *obj, unsigned size_bytes);

// verifast doesn't know about these.
// Skipped when libnf is embedded in libsycon (compiled inside `namespace libnf`):
// there these redeclarations would become libnf::__builtin_* and shadow the real
// compiler intrinsics at the call sites, leaving them undefined at link time.
#ifndef LIBNF_SKIP_BUILTIN_DECLS
unsigned __builtin_ia32_crc32si(unsigned acc, unsigned int x);
unsigned long long __builtin_ia32_crc32di(unsigned long long acc, unsigned long long x);
#endif

unsigned count_trailing_zeros(unsigned x);

// Returns the 1-indexed position of the least-significant set bit of x, or 0 if x
// is zero. Equivalently: the number of trailing zeros plus one, with zero mapped
// to zero (matching POSIX ffs). Examples:
//   find_first_set_bit(0b0001) = 1   (lowest bit set)
//   find_first_set_bit(0b1000) = 4   (bit 3 is the lowest set bit)
//   find_first_set_bit(0)      = 0   (no bits set)
unsigned find_first_set_bit(unsigned x);

// Returns the smaller of a and b.
unsigned min(unsigned a, unsigned b);

// Rotates the 32-bit value x left by n bits (n taken modulo 32).
unsigned rotate_left(unsigned x, unsigned n);

unsigned power_of_two(unsigned exponent);
unsigned divide(unsigned numerator, unsigned denominator);
// Natural logarithm with a caller-provided fixed-point scale: returns ln(x) * scale.
unsigned ln(unsigned x, unsigned scale);

// One CRC-32 configuration: the four parameters that define a CRC-32 variant (the "rocksoft"
// model with refin == refout). A width-W index is the low W bits of the 32-bit result.
struct crc32_config {
  const char *name;
  uint32_t coeff; // The polynomial in normal (non-reflected) form, x^32 implied.
  bool reversed;  // Reflect the input bytes and the output (refin = refout = true).
  uint32_t init;  // Initial register value.
  uint32_t xor_out;
};

// The polynomials the multi-row structures (bloom filters, count-min sketches) use, one per row:
// row i hashes with CRC32_BANK[i]. Rows MUST hash with different polynomials. Any CRC is affine
// over GF(2), so two keys that collide under one polynomial collide under the same polynomial
// whatever salt is appended, prepended or used as the initial value: salted copies of a single CRC
// are perfectly correlated rows. Two rows are as independent as their polynomials are coprime, so
// every entry is a PRIMITIVE polynomial (irreducible, hence pairwise coprime; verified by
// random_experiments/crc32_polynomials.py, which also generated entries 1-15). Entry 0 is the
// IEEE 802.3 CRC-32 (zlib's crc32), which the single-hash users keep using. Everything that must
// index these structures identically (the controllers, the switch programs) takes its
// configurations from this table.
#define CRC32_BANK_SIZE 16

static const struct crc32_config CRC32_BANK[CRC32_BANK_SIZE] = {
    {"ieee", 0x04C11DB7, true, 0xFFFFFFFF, 0xFFFFFFFF}, {"p1", 0x7B17A39F, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p2", 0x99F29AAD, true, 0xFFFFFFFF, 0xFFFFFFFF},   {"p3", 0x21BCA2C3, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p4", 0x0C596849, true, 0xFFFFFFFF, 0xFFFFFFFF},   {"p5", 0x3553D717, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p6", 0xAA1BC1D3, true, 0xFFFFFFFF, 0xFFFFFFFF},   {"p7", 0x297D4A93, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p8", 0xDE10F91F, true, 0xFFFFFFFF, 0xFFFFFFFF},   {"p9", 0xDC077BC9, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p10", 0xB5405445, true, 0xFFFFFFFF, 0xFFFFFFFF},  {"p11", 0x391E2DDD, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p12", 0x4E5ACD6D, true, 0xFFFFFFFF, 0xFFFFFFFF},  {"p13", 0x6AEEFABD, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p14", 0xFAF778B9, true, 0xFFFFFFFF, 0xFFFFFFFF},  {"p15", 0x99DFB91B, true, 0xFFFFFFFF, 0xFFFFFFFF},
};

// A table-driven hasher for one configuration.
struct crc32_hasher {
  struct crc32_config config;
  uint32_t table[256];
};

void crc32_hasher_init(struct crc32_hasher *hasher, const struct crc32_config *config);
uint32_t crc32_hasher_hash(const struct crc32_hasher *hasher, const void *data, size_t size);

#endif
