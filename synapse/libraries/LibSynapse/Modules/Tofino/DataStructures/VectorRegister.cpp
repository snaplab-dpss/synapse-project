#include <LibSynapse/Modules/Tofino/DataStructures/VectorRegister.h>

#include <cassert>
#include <iostream>

namespace LibSynapse {
namespace Tofino {

namespace {

std::vector<Register> build_registers(const TNAProperties &properties, DS_ID id, u32 capacity, bits_t index_size,
                                      const std::vector<bits_t> &values_sizes) {
  std::vector<Register> rows;

  for (size_t i = 0; i < values_sizes.size(); i++) {
    Register row(properties, id + "_" + std::to_string(i), capacity, index_size, values_sizes[i],
                 {RegisterActionType::WRITE, RegisterActionType::READ});
    rows.push_back(row);
  }

  return rows;
}

} // namespace

VectorRegister::VectorRegister(const TNAProperties &properties, DS_ID _id, u32 _capacity, bits_t _index_size,
                               const std::vector<bits_t> &_values_sizes)
    : DS(DSType::VECTOR_REGISTER, false, _id), capacity(_capacity), index_size(_index_size), values_sizes(_values_sizes),
      regs(build_registers(properties, _id, _capacity, _index_size, _values_sizes)) {}

VectorRegister::VectorRegister(const VectorRegister &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), index_size(other.index_size), values_sizes(other.values_sizes),
      regs(other.regs) {}

DS *VectorRegister::clone() const { return new VectorRegister(*this); }

void VectorRegister::debug() const {
  std::cerr << "\n";
  std::cerr << "========= VECTOR REGISTERS =======\n";
  std::cerr << "ID:     " << id << "\n";
  std::cerr << "Capacity: " << capacity << "\n";
  std::cerr << "Index size: " << index_size << "\n";
  std::cerr << "Values sizes: ";
  for (const bits_t &size : values_sizes) {
    std::cerr << size << " ";
  }
  std::cerr << "\n";
  std::cerr << "==================================\n";
}

std::vector<std::unordered_set<const DS *>> VectorRegister::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();
  for (const Register &reg : regs) {
    internal_ds.back().insert(&reg);
  }

  return internal_ds;
}

} // namespace Tofino
} // namespace LibSynapse