#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Register.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Hash.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>

namespace LibSynapse {
namespace Tofino {

constexpr const char *const FCFS_CACHED_TABLE_CACHE_SIZE_PARAM = "cache_size";

struct FCFSCachedTable : public DS {
  u32 cache_capacity;
  u32 capacity;
  std::vector<bits_t> keys_sizes;

  std::vector<Table> tables;
  Register reg_liveness;
  std::vector<Register> cache_keys;
  std::vector<Hash> hashes;

  static constexpr u32 MAX_CACHE_CAPACITY{65536};

  FCFSCachedTable(const tna_properties_t &properties, DS_ID id, u32 op, u32 cache_capacity, u32 capacity, const std::vector<bits_t> &keys_sizes);

  FCFSCachedTable(const FCFSCachedTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;

  bool has_table(u32 op) const;
  DS_ID add_table(u32 op);
  const Table *get_table(u32 op) const;
  const Table *get_table(const DS_ID &table_id) const;
  void remove_table(const DS_ID &table_id);

  bool has_hash(u32 op) const;
  DS_ID add_hash(u32 op);
  const Hash *get_hash(u32 op) const;
  const Hash *get_hash(const DS_ID &hash_id) const;
  void remove_hash(const DS_ID &hash_id);
};

} // namespace Tofino
} // namespace LibSynapse