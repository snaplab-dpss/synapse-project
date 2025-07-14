#include <LibSynapse/Modules/Tofino/DataStructures/FCFSCachedTable.h>

#include <cmath>
#include <iostream>

namespace LibSynapse {
namespace Tofino {

using LibCore::bits_from_pow2_capacity;

namespace {

std::string build_table_name(DS_ID id, u32 table_num) { return id + "_table_" + std::to_string(table_num); }

Register build_cache_expirator(const tna_properties_t &properties, DS_ID id, u32 cache_capacity) {
  const bits_t hash_size      = bits_from_pow2_capacity(cache_capacity);
  const bits_t timestamp_size = 32;
  return Register(properties, id + "_reg_expirator", cache_capacity, hash_size, timestamp_size, {RegisterActionType::Write});
}

std::vector<Register> build_cache_keys(const tna_properties_t &properties, DS_ID id, const std::vector<bits_t> &keys_sizes, u32 cache_capacity) {
  std::vector<Register> cache_keys;

  const bits_t hash_size = bits_from_pow2_capacity(cache_capacity);

  int i = 0;
  for (bits_t key_size : keys_sizes) {
    Register cache_key(properties, id + "_reg_key_" + std::to_string(i), cache_capacity, hash_size, key_size,
                       {RegisterActionType::Read, RegisterActionType::Swap});
    i++;
    cache_keys.push_back(cache_key);
  }

  return cache_keys;
}
} // namespace

FCFSCachedTable::FCFSCachedTable(const tna_properties_t &properties, DS_ID _id, u32 _op, u32 _cache_capacity, u32 _capacity,
                                 const std::vector<bits_t> &_keys_sizes)
    : DS(DSType::FCFSCachedTable, false, _id), cache_capacity(_cache_capacity), capacity(_capacity), keys_sizes(_keys_sizes),
      cache_expirator(build_cache_expirator(properties, id, cache_capacity)),
      cache_keys(build_cache_keys(properties, id, keys_sizes, cache_capacity)) {
  assert(cache_capacity > 0 && "Cache capacity must be greater than 0");
  assert(capacity > 0 && "Number of entries must be greater than 0");
  assert(cache_capacity <= capacity && "Cache capacity must be less than the "
                                       "number of entries");
  add_table(_op);
}

FCFSCachedTable::FCFSCachedTable(const FCFSCachedTable &other)
    : DS(other.type, other.primitive, other.id), cache_capacity(other.cache_capacity), capacity(other.capacity), keys_sizes(other.keys_sizes),
      tables(other.tables), cache_expirator(other.cache_expirator), cache_keys(other.cache_keys) {}

DS *FCFSCachedTable::clone() const { return new FCFSCachedTable(*this); }

void FCFSCachedTable::debug() const {
  std::cerr << "\n";
  std::cerr << "======== FCFS CACHED TABLE ========\n";
  std::cerr << "ID:      " << id << "\n";
  std::cerr << "Entries: " << capacity << "\n";
  std::cerr << "Cache:   " << cache_capacity << "\n";
  for (const Table &table : tables) {
    table.debug();
  }
  cache_expirator.debug();
  for (const Register &cache_key : cache_keys) {
    cache_key.debug();
  }
  std::cerr << "==============================\n";
}

std::vector<std::unordered_set<const DS *>> FCFSCachedTable::get_internal() const {
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
  Table new_table(build_table_name(id, op), capacity - cache_capacity, keys_sizes, {});
  tables.push_back(new_table);
  return new_table.id;
}

} // namespace Tofino
} // namespace LibSynapse