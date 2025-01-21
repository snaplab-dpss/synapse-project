#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

struct Hash : public DS {
  std::vector<bits_t> keys;

  Hash(DS_ID id, const std::vector<bits_t> &keys);
  Hash(const Hash &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_match_xbar_consume() const;
};

} // namespace Tofino
} // namespace LibSynapse