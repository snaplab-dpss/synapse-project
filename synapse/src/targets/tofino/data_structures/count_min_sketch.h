#pragma once

#include <vector>
#include <optional>

#include "data_structure.h"
#include "table.h"
#include "register.h"
#include "../../../types.h"
#include "../../../log.h"

namespace tofino {

struct CountMinSketch : public DS {
  u32 width;
  u32 height;

  // TODO: missing the hash calculators.
  std::vector<Register> rows;

  CountMinSketch(const TNAProperties &properties, DS_ID id, u32 _width,
                 u32 _height);

  CountMinSketch(const CountMinSketch &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;
};

} // namespace tofino
