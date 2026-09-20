#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibCore/Types.h>

#include <array>
#include <vector>
#include <optional>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

// One CRC-32 configuration: the four parameters that define a CRC-32 variant (the "rocksoft"
// model with refin == refout). The hash unit's width-W output is the low W bits of the 32-bit
// result. The emitter maps these onto TNA's CRCPolynomial extern (TofinoSynthesizer::
// crc_polynomial_args).
struct crc32_config_t {
  const char *name;
  u32 coeff;     // The polynomial in normal (non-reflected) form, x^32 implied.
  bool reversed; // Reflect the input bytes and the output (refin = refout = true).
  u32 init;      // Initial register value.
  u32 xor_out;
};

// The polynomials the multi-row structures (bloom filters, count-min sketches, cuckoo tables) use,
// one per row: row i hashes with CRC32_BANK[i]. Rows MUST hash with different polynomials. Any CRC
// is affine over GF(2), so two keys that collide under one polynomial collide under the same
// polynomial whatever salt is appended, prepended or used as the initial value: salted copies of a
// single CRC are perfectly correlated rows. Two rows are as independent as their polynomials are
// coprime, so every entry is a primitive polynomial. Entry 0 is the IEEE 802.3 CRC-32 (TNA's
// built-in HashAlgorithm_t.CRC32), which the single-hash structures keep using.
//
// This table MUST equal libnf's CRC32_BANK (dpdk-nfs/lib/util/crc32.h), which the controllers and
// the C NFs index with: random_experiments/crc32_polynomials.py checks both and their primitivity.
constexpr size_t CRC32_BANK_SIZE = 16;

constexpr std::array<crc32_config_t, CRC32_BANK_SIZE> CRC32_BANK = {{
    {"ieee", 0x04C11DB7, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p1", 0x7B17A39F, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p2", 0x99F29AAD, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p3", 0x21BCA2C3, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p4", 0x0C596849, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p5", 0x3553D717, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p6", 0xAA1BC1D3, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p7", 0x297D4A93, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p8", 0xDE10F91F, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p9", 0xDC077BC9, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p10", 0xB5405445, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p11", 0x391E2DDD, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p12", 0x4E5ACD6D, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p13", 0x6AEEFABD, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p14", 0xFAF778B9, true, 0xFFFFFFFF, 0xFFFFFFFF},
    {"p15", 0x99DFB91B, true, 0xFFFFFFFF, 0xFFFFFFFF},
}};

struct Hash : public DS {
  std::vector<bits_t> keys;
  bits_t size;
  // The CRC-32 the hash unit computes, one of CRC32_BANK. Entry 0 is the IEEE polynomial (TNA's
  // built-in CRC32); the rows of a multi-row structure each take a different entry, since rows
  // sharing one CRC collide on exactly the same key pairs whatever salt they are given.
  crc32_config_t polynomial;

  Hash(DS_ID id, const std::vector<bits_t> &keys, bits_t size, const crc32_config_t &polynomial = CRC32_BANK[0]);
  Hash(const Hash &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_match_xbar_consume() const;
};

} // namespace Tofino
} // namespace LibSynapse