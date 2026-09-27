#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

// A longest-prefix-match table: a ternary match on the key's fields, an entry per prefix with the
// prefix's mask and a priority by its length, and one action returning the 32-bit value.
struct LPM : public DS {
  u32 capacity;
  bits_t key;
  std::vector<bits_t> keys_size; // the key's fields, as the table matches them
  bool dns_name_key;             // the key is a DNS name in the parser's piece layout (DnsGetResponse)

  LPM(DS_ID id, u32 capacity, const std::vector<bits_t> &keys_size, bool dns_name_key);
  LPM(const LPM &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_match_xbar_consume() const;
  bits_t get_consumed_tcam() const;
};

} // namespace Tofino
} // namespace LibSynapse