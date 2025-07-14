#pragma once

#include <LibCore/Types.h>
#include <LibCore/Expr.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>

#include <vector>
#include <optional>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibCore::expr_struct_t;

enum class TimeAware { Yes, No };

struct Table : public DS {
  u32 capacity;
  std::vector<bits_t> keys;
  std::vector<bits_t> params;
  TimeAware time_aware;

  Table(DS_ID id, u32 capacity, const std::vector<bits_t> &keys, const std::vector<bits_t> &params, TimeAware time_aware = TimeAware::No);
  Table(const Table &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_match_xbar_consume() const;
  bits_t get_consumed_sram() const;
  bits_t get_consumed_sram_per_entry() const;

  static u32 adjust_capacity_for_collisions(u32 capacity);
  static std::vector<klee::ref<klee::Expr>> build_keys(klee::ref<klee::Expr> key, const std::vector<expr_struct_t> &headers);
};

} // namespace Tofino
} // namespace LibSynapse