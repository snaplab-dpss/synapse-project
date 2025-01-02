#pragma once

#include <vector>
#include <optional>

#include "data_structure.h"
#include "table.h"
#include "register.h"
#include "../../../types.h"
#include "../../../log.h"

#define CACHE_SIZE_PARAM "cache_size"

namespace synapse {
namespace tofino {

struct FCFSCachedTable : public DS {
  u32 cache_capacity;
  u32 num_entries;
  std::vector<bits_t> keys_sizes;

  std::vector<Table> tables;
  Register cache_expirator;
  std::vector<Register> cache_keys;

  FCFSCachedTable(const TNAProperties &properties, DS_ID id, u32 op, u32 cache_capacity,
                  u32 num_entries, const std::vector<bits_t> &keys_sizes);

  FCFSCachedTable(const FCFSCachedTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;

  bool has_table(u32 op) const;
  std::optional<DS_ID> add_table(u32 op);
};

} // namespace tofino
} // namespace synapse