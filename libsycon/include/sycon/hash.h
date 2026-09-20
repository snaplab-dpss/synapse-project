#pragma once

#include "buffer.h"
#include "libnf.h"

#include <cassert>
#include <vector>

namespace sycon {

// A CRC-32 with one of the configurations in libnf's CRC32_BANK (lib/util/crc32.h): the same
// table-driven hasher libnf uses, so the controller and the C NFs index identically, and the
// configurations synapse emits the P4 CRCPolynomial externs from, so the dataplane does too (a
// width-W index is the low W bits on both sides; verified in tofino/exp-hash). The default is
// entry 0, the IEEE 802.3 CRC-32 (TNA's built-in HashAlgorithm_t.CRC32).
class CRC32 {
private:
  libnf::crc32_hasher hasher;

public:
  explicit CRC32(const libnf::crc32_config &config = libnf::CRC32_BANK[0]) { libnf::crc32_hasher_init(&hasher, &config); }

  u32 hash(const buffer_t &input) const { return libnf::crc32_hasher_hash(&hasher, input.data, input.size); }

  // One hasher per row of a multi-row structure (bloom filter, count-min sketch): row i hashes with
  // CRC32_BANK[i]. Rows must use different polynomials, since any CRC is affine and salting a single
  // one only permutes each row's cells while keeping which keys collide (see crc32.h).
  static std::vector<CRC32> per_row(size_t rows) {
    assert(rows <= CRC32_BANK_SIZE && "Not enough CRC polynomials for the number of rows");
    std::vector<CRC32> hashers;
    for (size_t i = 0; i < rows; i++) {
      hashers.emplace_back(libnf::CRC32_BANK[i]);
    }
    return hashers;
  }
};

} // namespace sycon
