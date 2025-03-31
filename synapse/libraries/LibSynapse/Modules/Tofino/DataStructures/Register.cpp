#include <LibSynapse/Modules/Tofino/DataStructures/Register.h>
#include <LibSynapse/Modules/Tofino/TNA/TNA.h>

#include <iostream>
#include <cassert>

namespace LibSynapse {
namespace Tofino {

Register::Register(const tna_properties_t &properties, DS_ID _id, u32 _capacity, bits_t _index_size, bits_t _value_size,
                   const std::unordered_set<RegisterActionType> &_actions)
    : DS(DSType::REGISTER, true, _id), capacity(_capacity), index_size(_index_size), value_size(_value_size), actions(_actions) {
  assert(_capacity > 0 && "Register entries must be greater than 0");
  assert(value_size <= properties.max_salu_size && "Register value exceeds SALU size");
}

Register::Register(const Register &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), index_size(other.index_size), value_size(other.value_size),
      actions(other.actions) {}

DS *Register::clone() const { return new Register(*this); }

bits_t Register::get_consumed_sram() const {
  bits_t index_sram_block = 1024 * 128;
  return value_size * capacity + index_sram_block;
}

u32 Register::get_num_logical_ids() const { return (u32)actions.size(); }

void Register::debug() const {
  std::cerr << "\n";
  std::cerr << "========== REGISTER ==========\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << capacity << "\n";

  std::stringstream ss;

  ss << "Actions:  [";
  bool first = true;
  for (RegisterActionType action : actions) {
    if (!first) {
      ss << ", ";
    }
    switch (action) {
    case RegisterActionType::READ:
      ss << "READ";
      break;
    case RegisterActionType::WRITE:
      ss << "WRITE";
      break;
    case RegisterActionType::SWAP:
      ss << "SWAP";
      break;
    case RegisterActionType::INC:
      ss << "INC";
      break;
    case RegisterActionType::DEC:
      ss << "DEC";
      break;
    case RegisterActionType::SET_TO_ONE_AND_RETURN_OLD_VALUE:
      ss << "SET_TO_ONE_AND_RETURN_OLD_VALUE";
      break;
    case RegisterActionType::INC_AND_RETURN_NEW_VALUE:
      ss << "INC_AND_RETURN_NEW_VALUE";
      break;
    case RegisterActionType::CALCULATE_DIFF:
      ss << "CALCULATE_DIFF";
      break;
    }
    first = false;
  }
  ss << "]\n";

  std::cerr << ss.str();
  std::cerr << "Index sz:  " << index_size << "b\n";
  std::cerr << "Value sz:  " << value_size << "b\n";
  std::cerr << "SRAM:      " << get_consumed_sram() / 8 << " B\n";
  std::cerr << "==============================\n";
}

std::vector<klee::ref<klee::Expr>> Register::partition_value(const tna_properties_t &properties, klee::ref<klee::Expr> value,
                                                             const std::vector<LibCore::expr_struct_t> &expr_structs) {
  const bits_t max_partition_width = properties.max_salu_size;
  const std::vector<klee::ref<klee::Expr>> partitions =
      LibCore::break_expr_into_structs_aware_chunks(value, expr_structs, max_partition_width / 8);
  return partitions;
}

} // namespace Tofino
} // namespace LibSynapse