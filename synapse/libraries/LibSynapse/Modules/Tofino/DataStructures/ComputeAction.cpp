#include <LibSynapse/Modules/Tofino/DataStructures/ComputeAction.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

ComputeAction::ComputeAction(DS_ID _id, const std::vector<compute_op_t> &_ops) : DS(DSType::ComputeAction, true, _id), ops(_ops) {}

ComputeAction::ComputeAction(const ComputeAction &other) : DS(other.type, other.primitive, other.id), ops(other.ops) {}

DS *ComputeAction::clone() const { return new ComputeAction(*this); }

int ComputeAction::hash_dist_units_for(bits_t width) { return static_cast<int>((width + HASH_DIST_UNIT_BITS - 1) / HASH_DIST_UNIT_BITS); }

bool ComputeAction::can_take(const compute_op_t &op) const {
  if (op.kind == ComputeOpKind::Hash && get_hash_bits() + op.width > MAX_HASH_BITS_PER_ACTION) {
    return false;
  }
  return true;
}

bits_t ComputeAction::get_hash_bits() const {
  bits_t bits = 0;
  for (const compute_op_t &op : ops) {
    if (op.kind == ComputeOpKind::Hash) {
      bits += op.width;
    }
  }
  return bits;
}

int ComputeAction::get_hash_dist_units() const {
  int units = 0;
  for (const compute_op_t &op : ops) {
    if (op.kind == ComputeOpKind::Hash) {
      units += hash_dist_units_for(op.width);
    }
  }
  return units;
}

size_t ComputeAction::get_num_alu_ops() const {
  size_t n = 0;
  for (const compute_op_t &op : ops) {
    n += (op.kind == ComputeOpKind::ALU);
  }
  return n;
}

size_t ComputeAction::get_num_hash_ops() const { return ops.size() - get_num_alu_ops(); }

void ComputeAction::debug() const {
  std::cerr << "\n";
  std::cerr << "======= COMPUTE ACTION =======\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Ops:       [";
  for (const compute_op_t &op : ops) {
    std::cerr << op.id << (op.kind == ComputeOpKind::Hash ? "(hash," : "(alu,") << op.width << "b) ";
  }
  std::cerr << "]\n";
  std::cerr << "ALU ops:   " << get_num_alu_ops() << "\n";
  std::cerr << "Hash ops:  " << get_num_hash_ops() << " (" << get_hash_bits() << " b, " << get_hash_dist_units() << " dist units)\n";
  std::cerr << "==============================\n";
}

} // namespace Tofino
} // namespace LibSynapse
