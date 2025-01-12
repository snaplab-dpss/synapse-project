#pragma once

#include "tree.h"
#include "nodes/nodes.h"
#include "profile.h"
#include "visitors/visitors.h"
#include "reorder.h"
#include "symbex.h"

namespace synapse {

struct branch_direction_t {
  const Branch *branch;
  bool direction;

  branch_direction_t() : branch(nullptr), direction(false) {}
  branch_direction_t(const branch_direction_t &other)            = default;
  branch_direction_t(branch_direction_t &&other)                 = default;
  branch_direction_t &operator=(const branch_direction_t &other) = default;
};

struct map_rw_pattern_t {
  const Call *map_get;
  branch_direction_t map_get_success_check;
  branch_direction_t map_put_extra_condition;
  const Call *dchain_allocate_new_index;
  branch_direction_t index_alloc_check;
  const Call *map_put;
};

} // namespace synapse