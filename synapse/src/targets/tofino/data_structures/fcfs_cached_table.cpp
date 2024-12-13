#include <math.h>

#include "fcfs_cached_table.h"

namespace tofino {

static bits_t index_size_from_cache_capacity(u32 cache_capacity) {
  // Log base 2 of the cache capacity
  // Assert cache capacity is a power of 2
  ASSERT((cache_capacity & (cache_capacity - 1)) == 0,
         "Cache capacity must be a power of 2");
  return bits_t(log2(cache_capacity));
}

static std::string build_table_name(DS_ID id, u32 table_num) {
  return id + "_table_" + std::to_string(table_num);
}

static Register build_cache_expirator(const TNAProperties &properties, DS_ID id,
                                      u32 cache_capacity) {
  bits_t hash_size = index_size_from_cache_capacity(cache_capacity);
  bits_t timestamp_size = 32;
  return Register(properties, id + "_reg_expirator", cache_capacity, hash_size,
                  timestamp_size, {RegisterAction::WRITE});
}

static std::vector<Register>
build_cache_keys(const TNAProperties &properties, DS_ID id,
                 const std::vector<bits_t> &keys_sizes, u32 cache_capacity) {
  std::vector<Register> cache_keys;

  bits_t hash_size = index_size_from_cache_capacity(cache_capacity);

  int i = 0;
  for (bits_t key_size : keys_sizes) {
    Register cache_key(properties, id + "_reg_key_" + std::to_string(i),
                       cache_capacity, hash_size, key_size,
                       {RegisterAction::READ, RegisterAction::SWAP});
    i++;
    cache_keys.push_back(cache_key);
  }

  return cache_keys;
}

FCFSCachedTable::FCFSCachedTable(const TNAProperties &properties, DS_ID _id,
                                 u32 _op, u32 _cache_capacity, u32 _num_entries,
                                 const std::vector<bits_t> &_keys_sizes)
    : DS(DSType::FCFS_CACHED_TABLE, false, _id),
      cache_capacity(_cache_capacity), num_entries(_num_entries),
      keys_sizes(_keys_sizes),
      cache_expirator(build_cache_expirator(properties, _id, cache_capacity)),
      cache_keys(build_cache_keys(properties, id, keys_sizes, cache_capacity)) {
  ASSERT(cache_capacity > 0, "Cache capacity must be greater than 0");
  ASSERT(num_entries > 0, "Number of entries must be greater than 0");
  ASSERT(cache_capacity < num_entries, "Cache capacity must be less than the "
                                       "number of entries");

  add_table(_op);
}

FCFSCachedTable::FCFSCachedTable(const FCFSCachedTable &other)
    : DS(other.type, other.primitive, other.id),
      cache_capacity(other.cache_capacity), num_entries(other.num_entries),
      keys_sizes(other.keys_sizes), tables(other.tables),
      cache_expirator(other.cache_expirator), cache_keys(other.cache_keys) {}

DS *FCFSCachedTable::clone() const { return new FCFSCachedTable(*this); }

void FCFSCachedTable::debug() const {
  Log::dbg() << "\n";
  Log::dbg() << "======== FCFS CACHED TABLE ========\n";
  Log::dbg() << "ID:      " << id << "\n";
  Log::dbg() << "Entries: " << num_entries << "\n";
  Log::dbg() << "Cache:   " << cache_capacity << "\n";
  for (const Table &table : tables) {
    table.debug();
  }
  cache_expirator.debug();
  for (const Register &cache_key : cache_keys) {
    cache_key.debug();
  }
  Log::dbg() << "==============================\n";
}

std::vector<std::unordered_set<const DS *>>
FCFSCachedTable::get_internal() const {
  // Access to the table comes first, then the expirator, and finally the keys.

  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();
  for (const Table &table : tables)
    internal_ds.back().insert(&table);

  internal_ds.emplace_back();
  internal_ds.back().insert(&cache_expirator);

  internal_ds.emplace_back();
  for (const Register &cache_key : cache_keys)
    internal_ds.back().insert(&cache_key);

  return internal_ds;
}

bool FCFSCachedTable::has_table(u32 op) const {
  std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return true;
  }
  return false;
}

std::optional<DS_ID> FCFSCachedTable::add_table(u32 op) {
  Table new_table(build_table_name(id, op), num_entries - cache_capacity,
                  keys_sizes, {});
  tables.push_back(new_table);
  return new_table.id;
}

} // namespace tofino
