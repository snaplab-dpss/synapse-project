#pragma once

#include <vector>
#include <optional>

#include <klee/Expr.h>

#include "data_structure.hpp"
#include "../../../types.hpp"
#include "../../../system.hpp"

namespace synapse {
namespace tofino {

struct Hash : public DS {
  std::vector<bits_t> keys;

  Hash(DS_ID id, const std::vector<bits_t> &keys);
  Hash(const Hash &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_match_xbar_consume() const;
};

} // namespace tofino
} // namespace synapse