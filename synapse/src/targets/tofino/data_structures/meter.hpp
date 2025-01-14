#pragma once

#include <vector>
#include <optional>

#include <klee/Expr.h>

#include "data_structure.hpp"
#include "../../../types.hpp"
#include "../../../system.hpp"

namespace synapse {
namespace tofino {

struct Meter : public DS {
  u32 capacity;
  Bps_t rate;
  bytes_t burst;
  std::vector<bits_t> keys;

  Meter(DS_ID id, u32 capacity, Bps_t rate, bytes_t burst, const std::vector<bits_t> &keys);

  Meter(const Meter &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_match_xbar_consume() const;
  bits_t get_consumed_sram() const;

  static std::vector<klee::ref<klee::Expr>> build_keys(klee::ref<klee::Expr> key);
};

} // namespace tofino
} // namespace synapse