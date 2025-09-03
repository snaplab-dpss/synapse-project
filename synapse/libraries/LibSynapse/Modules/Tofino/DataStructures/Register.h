#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibCore/Types.h>
#include <LibCore/Expr.h>

#include <vector>
#include <optional>
#include <unordered_set>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibCore::expr_struct_t;

struct tna_properties_t;

enum class RegisterActionType {
  Read,
  Write,
  Swap,
  Increment,
  Decrement,
  SetToOneAndReturnOldValue,
  IncrementAndReturnNewValue,
  ReadConditionalWrite,
  CalculateDiff,
  QueryAndRefreshTimestamp,
};

const std::unordered_set<RegisterActionType> register_action_types_with_out_value = {
    RegisterActionType::Read,
    RegisterActionType::Swap,
    RegisterActionType::SetToOneAndReturnOldValue,
    RegisterActionType::IncrementAndReturnNewValue,
    RegisterActionType::ReadConditionalWrite,
    RegisterActionType::CalculateDiff,
    RegisterActionType::QueryAndRefreshTimestamp,
};

struct Register : public DS {
  u32 capacity;
  bits_t index_size;
  bits_t value_size;
  std::unordered_set<RegisterActionType> actions;

  Register(const tna_properties_t &properties, DS_ID id, u32 capacity, bits_t index_size, bits_t value_size,
           const std::unordered_set<RegisterActionType> &actions);

  Register(const Register &other);

  DS *clone() const override;
  void debug() const override;

  bits_t get_consumed_sram() const;
  u32 get_num_logical_ids() const;

  static std::vector<klee::ref<klee::Expr>> partition_value(const tna_properties_t &properties, klee::ref<klee::Expr> value,
                                                            const std::vector<expr_struct_t> &expr_structs);
};

} // namespace Tofino
} // namespace LibSynapse