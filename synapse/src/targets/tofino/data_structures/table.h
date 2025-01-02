#pragma once

#include <vector>
#include <optional>

#include <klee/Expr.h>

#include "data_structure.h"
#include "../../../types.h"
#include "../../../log.h"

namespace synapse {
namespace tofino {

struct Table : public DS {
  u32 num_entries;
  std::vector<bits_t> keys;
  std::vector<bits_t> params;

  Table(DS_ID id, u32 num_entries, const std::vector<bits_t> &keys,
        const std::vector<bits_t> &params);

  Table(const Table &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_match_xbar_consume() const;
  bits_t get_consumed_sram() const;

  static std::vector<klee::ref<klee::Expr>> build_keys(klee::ref<klee::Expr> key);
};

} // namespace tofino
} // namespace synapse