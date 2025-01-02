#pragma once

#include <vector>
#include <optional>

#include <klee/Expr.h>

#include "data_structure.h"
#include "../../../types.h"
#include "../../../log.h"

namespace synapse {
namespace tofino {

struct TNAProperties;

enum class RegisterAction {
  READ,  // No modification
  WRITE, // Overwrites the current value
  SWAP,  // Returns the old value
};

struct Register : public DS {
  u32 num_entries;
  bits_t index;
  bits_t value;
  std::unordered_set<RegisterAction> actions;

  Register(const TNAProperties &properties, DS_ID id, u32 num_entries, bits_t index,
           bits_t value, const std::unordered_set<RegisterAction> &actions);

  Register(const Register &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_consumed_sram() const;
  u32 get_num_logical_ids() const;

  static std::vector<klee::ref<klee::Expr>>
  partition_value(const TNAProperties &properties, klee::ref<klee::Expr> value);
};

} // namespace tofino
} // namespace synapse