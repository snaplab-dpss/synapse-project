#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibCore/Types.h>

#include <cmath>
#include <set>
#include <utility>
#include <vector>

namespace LibSynapse {
namespace Tofino {

// Const-entry generators for the compute-lookup tables. Each function's table is
// populated from its own closed-form definition (ctz/ffs/power_of_two) or from
// the values the profiler observed (ln). ctz/ffs use ternary leading-bit patterns;
// power_of_two/ln use exact match.

// count_trailing_zeros: for a value whose lowest set bit is at position k, match
// (2^k &&& 2^(k+1)-1) -> k. (Default/no-match is handled as 0 by the table.)
inline std::vector<table_entry_t> count_trailing_zeros_entries(bits_t in_width) {
  std::vector<table_entry_t> entries;
  for (bits_t k = 0; k < in_width && k < 64; k++) {
    const u64 bit  = 1ull << k;
    const u64 mask = (k == 63) ? ~0ull : ((1ull << (k + 1)) - 1);
    entries.push_back({bit, mask, static_cast<u64>(k)});
  }
  return entries;
}

// find_first_set_bit: same leading-bit patterns as ctz, but the value is the
// 1-indexed position (k + 1). No match -> 0 (the default action).
inline std::vector<table_entry_t> find_first_set_bit_entries(bits_t in_width) {
  std::vector<table_entry_t> entries;
  for (bits_t k = 0; k < in_width && k < 64; k++) {
    const u64 bit  = 1ull << k;
    const u64 mask = (k == 63) ? ~0ull : ((1ull << (k + 1)) - 1);
    entries.push_back({bit, mask, static_cast<u64>(k) + 1});
  }
  return entries;
}

// power_of_two: exact entry e -> 2^e for every exponent that fits the output.
inline std::vector<table_entry_t> power_of_two_entries(bits_t out_width) {
  std::vector<table_entry_t> entries;
  for (bits_t e = 0; e < out_width && e < 64; e++) {
    entries.push_back({static_cast<u64>(e), ~0ull, 1ull << e});
  }
  return entries;
}

// ln: exact entry x -> round(ln(x) * scale), for each (x, scale) the profiler saw.
inline std::vector<table_entry_t> ln_entries(const std::set<std::pair<u32, u32>> &inputs) {
  std::vector<table_entry_t> entries;
  for (const auto &[x, scale] : inputs) {
    const u64 value = (x == 0) ? 0 : static_cast<u64>(std::log(static_cast<double>(x)) * scale);
    entries.push_back({static_cast<u64>(x), ~0ull, value});
  }
  return entries;
}

} // namespace Tofino
} // namespace LibSynapse
