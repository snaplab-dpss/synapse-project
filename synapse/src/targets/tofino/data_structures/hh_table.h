#pragma once

#include <vector>
#include <optional>

#include <klee/Expr.h>

#include "data_structure.h"
#include "count_min_sketch.h"
#include "table.h"

#include "../../../types.h"
#include "../../../log.h"

namespace tofino {

struct HHTable : public DS {
  u32 num_entries;
  std::vector<bits_t> keys_sizes;
  std::vector<Table> tables;

  u32 cms_width;
  u32 cms_height;
  CountMinSketch cms;

  HHTable(const TNAProperties &properties, DS_ID id, u32 op, u32 num_entries,
          const std::vector<bits_t> &keys_sizes, u32 cms_width, u32 cms_height);

  HHTable(const HHTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;

  bool has_table(u32 op) const;
  DS_ID add_table(u32 op);
};

} // namespace tofino
