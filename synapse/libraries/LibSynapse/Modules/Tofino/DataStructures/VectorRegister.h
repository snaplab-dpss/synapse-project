#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Register.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>
#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

struct VectorRegister : public DS {
  u32 capacity;
  bits_t index_size;
  std::vector<bits_t> values_sizes;
  std::vector<Register> regs;

  VectorRegister(const TNAProperties &properties, DS_ID id, u32 capacity, bits_t index_size, const std::vector<bits_t> &values_sizes);
  VectorRegister(const VectorRegister &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;
};

} // namespace Tofino
} // namespace LibSynapse