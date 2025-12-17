#pragma once

#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Register.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Hash.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Digest.h>
#include <LibCore/Types.h>

#include <vector>
#include <optional>

namespace LibSynapse {
namespace Tofino {

constexpr const char *const FCFS_CACHED_TABLE_CACHE_SIZE_PARAM = "cache_size";

struct FCFSCachedTable : public DS {
  static constexpr const u32 ENTRY_TIMEOUT{16384}; // 1 s (in units of 65536 ns)

  u32 cache_capacity;
  u32 capacity;
  std::vector<bits_t> keys_sizes;

  Hash hash;
  std::vector<Table> tables;
  Register cache_expirator;
  std::vector<Register> cache_keys;
  Digest digest;

  FCFSCachedTable(const tna_properties_t &properties, DS_ID id, u32 op, u32 cache_capacity, u32 capacity, const std::vector<bits_t> &keys_sizes,
                  u8 digest_type);

  FCFSCachedTable(const FCFSCachedTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;

  bool has_table(u32 op) const;
  std::optional<DS_ID> add_table(u32 op);
  const Table *get_table(u32 op) const;
  const Table *get_table(const DS_ID &table_id) const;
  void remove_table(const DS_ID &table_id);
};

} // namespace Tofino
} // namespace LibSynapse