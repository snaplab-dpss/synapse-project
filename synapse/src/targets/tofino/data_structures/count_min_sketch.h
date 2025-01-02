#pragma once

#include <vector>
#include <optional>

#include "data_structure.h"
#include "table.h"
#include "register.h"
#include "hash.h"
#include "../../../types.h"
#include "../../../log.h"

namespace synapse {
namespace tofino {

struct CountMinSketch : public DS {
  u32 width;
  u32 height;

  std::vector<Hash> hashes;
  std::vector<Register> rows;

  CountMinSketch(const TNAProperties &properties, DS_ID id,
                 const std::vector<bits_t> &keys, u32 width, u32 height);

  CountMinSketch(const CountMinSketch &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;
};

} // namespace tofino
} // namespace synapse