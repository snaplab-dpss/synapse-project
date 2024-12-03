#pragma once

#include <vector>
#include <optional>

#include "data_structure.h"
#include "register.h"
#include "../../../types.h"
#include "../../../log.h"

namespace tofino {

struct MapRegister : public DS {
  u32 num_entries;

  Register expirator;
  std::vector<Register> keys;

  MapRegister(const TNAProperties &properties, DS_ID id, u32 num_entries,
              const std::vector<bits_t> &keys_sizes);

  MapRegister(const MapRegister &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;
};

} // namespace tofino
