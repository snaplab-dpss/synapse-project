#pragma once

#include <vector>
#include <optional>

#include <klee/Expr.h>

#include "data_structure.h"
#include "../../../types.h"
#include "../../../system.h"

namespace synapse {
namespace tofino {

struct TNAProperties;

enum class RegisterActionType {
  READ,  // No modification
  WRITE, // Overwrites the current value
  SWAP,  // Overwrites the current value and returns the old one
};

struct Register : public DS {
  u32 num_entries;
  bits_t index_size;
  bits_t value_size;
  std::unordered_set<RegisterActionType> actions;

  Register(const TNAProperties &properties, DS_ID id, u32 num_entries, bits_t index_size, bits_t value_size,
           const std::unordered_set<RegisterActionType> &actions);

  Register(const Register &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_consumed_sram() const;
  u32 get_num_logical_ids() const;

  static std::vector<klee::ref<klee::Expr>> partition_value(const TNAProperties &properties,
                                                            klee::ref<klee::Expr> value);
};

} // namespace tofino
} // namespace synapse