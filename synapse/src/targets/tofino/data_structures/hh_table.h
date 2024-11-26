#pragma once

#include <vector>
#include <optional>

#include <klee/Expr.h>

#include "data_structure.h"
#include "../../../types.h"
#include "../../../log.h"

namespace tofino {

struct HHTable : public DS {
  int num_entries;
  int cms_width;
  int cms_height;

  HHTable(DS_ID id, int op, int num_entries, int cms_width, int cms_height,
          const std::vector<bits_t> &keys);

  HHTable(const HHTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;
};

} // namespace tofino
