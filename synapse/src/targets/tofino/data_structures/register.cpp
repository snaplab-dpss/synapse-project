#include "register.h"
#include "../tna/tna.h"

namespace synapse {
namespace tofino {

Register::Register(const TNAProperties &properties, DS_ID _id, u32 _num_entries, bits_t _index_size, bits_t _value_size,
                   const std::unordered_set<RegisterActionType> &_actions)
    : DS(DSType::REGISTER, true, _id), num_entries(_num_entries), index_size(_index_size), value_size(_value_size),
      actions(_actions) {
  assert(_num_entries > 0 && "Register entries must be greater than 0");
  assert(value_size <= properties.max_salu_size && "Register value exceeds SALU size");
}

Register::Register(const Register &other)
    : DS(other.type, other.primitive, other.id), num_entries(other.num_entries), index_size(other.index_size),
      value_size(other.value_size), actions(other.actions) {}

DS *Register::clone() const { return new Register(*this); }

bits_t Register::get_consumed_sram() const {
  bits_t index_sram_block = 1024 * 128;
  return value_size * num_entries + index_sram_block;
}

u32 Register::get_num_logical_ids() const { return (u32)actions.size(); }

void Register::debug() const {
  std::cerr << "\n";
  std::cerr << "========== REGISTER ==========\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << num_entries << "\n";

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

std::vector<klee::ref<klee::Expr>> Register::partition_value(const TNAProperties &properties,
                                                             klee::ref<klee::Expr> value) {
  std::vector<klee::ref<klee::Expr>> partitions;

  bits_t value_width     = value->getWidth();
  bits_t partition_width = properties.max_salu_size;

  bits_t offset = 0;
  while (offset < value_width) {
    if (offset + partition_width > value_width) {
      partition_width = value_width - offset;
    }

    klee::ref<klee::Expr> partition = solver_toolbox.exprBuilder->Extract(value, offset, partition_width);
    partitions.push_back(partition);

    offset += partition_width;
  }

  return partitions;
}

} // namespace tofino
} // namespace synapse