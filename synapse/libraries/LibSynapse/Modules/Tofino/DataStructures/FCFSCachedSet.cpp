#include <LibSynapse/Modules/Tofino/DataStructures/FCFSCachedSet.h>
#include <LibSynapse/Modules/Tofino/TNA/TNAProperties.h>

#include <cmath>
#include <iostream>

namespace LibSynapse {
namespace Tofino {

using LibCore::bits_from_pow2_capacity;

namespace {

DS_ID build_table_name(DS_ID id, u32 table_num) { return id + "_table_" + std::to_string(table_num); }

DS_ID build_hash_name(DS_ID id, u32 hash_num) { return id + "_hash_" + std::to_string(hash_num); }

Register build_reg_liveness(const tna_properties_t &properties, DS_ID id, u32 cache_capacity) {
  const bits_t hash_size  = bits_from_pow2_capacity(cache_capacity);
  const bits_t match_size = 8;
  return Register(properties, id + "_reg_liveness", cache_capacity, hash_size, match_size,
                  {RegisterActionType::QueryTimestamp, RegisterActionType::QueryAndRefreshTimestamp});
}

std::vector<Register> build_cache_keys(const tna_properties_t &properties, DS_ID id, const std::vector<bits_t> &elements_sizes, u32 capacity) {
  std::vector<Register> registers;

  const bits_t hash_size = bits_from_pow2_capacity(capacity);

  int i = 0;
  for (bits_t key_size : elements_sizes) {
    Register cache_key(properties, id + "_reg_key_" + std::to_string(i), capacity, hash_size, key_size,
                       {RegisterActionType::CheckValue, RegisterActionType::Write});
    i++;
    registers.push_back(cache_key);
  }

  return registers;
}

} // namespace

FCFSCachedSet::FCFSCachedSet(const tna_properties_t &properties, DS_ID _id, u32 _op, u32 _cache_capacity, u32 _capacity,
                             const std::vector<bits_t> &_keys_sizes)
    : DS(DSType::FCFSCachedSet, false, _id), cache_capacity(_cache_capacity), capacity(_capacity), keys_sizes(_keys_sizes),
      reg_liveness(build_reg_liveness(properties, id, cache_capacity)), cache_keys(build_cache_keys(properties, id, keys_sizes, cache_capacity)) {
  assert(cache_capacity > 0);
  assert(capacity > 0);
  assert(cache_capacity < capacity);
  add_table(_op);
  add_hash(_op);
}

FCFSCachedSet::FCFSCachedSet(const FCFSCachedSet &other)
    : DS(other.type, other.primitive, other.id), cache_capacity(other.cache_capacity), capacity(other.capacity), keys_sizes(other.keys_sizes),
      tables(other.tables), reg_liveness(other.reg_liveness), cache_keys(other.cache_keys), hashes(other.hashes) {}

DS *FCFSCachedSet::clone() const { return new FCFSCachedSet(*this); }

void FCFSCachedSet::debug() const {
  std::cerr << "\n";
  std::cerr << "======== FCFS CACHED TABLE ========\n";
  std::cerr << "ID:      " << id << "\n";
  std::cerr << "Entries: " << capacity << "\n";
  std::cerr << "Cache:   " << cache_capacity << "\n";
  for (const Table &table : tables) {
    table.debug();
  }
  reg_liveness.debug();
  for (const Register &cache_key : cache_keys) {
    cache_key.debug();
  }
  for (const Hash &hash : hashes) {
    hash.debug();
  }
  std::cerr << "==============================\n";
}

std::vector<std::unordered_set<const DS *>> FCFSCachedSet::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();
  for (const Table &table : tables) {
    internal_ds.back().insert(&table);
  }

  internal_ds.emplace_back();
  for (const Hash &hash : hashes) {
    internal_ds.back().insert(&hash);
  }
  internal_ds.back().insert(&reg_liveness);

  internal_ds.emplace_back();
  for (const Register &cache_key : cache_keys) {
    internal_ds.back().insert(&cache_key);
  }

  return internal_ds;
}

bool FCFSCachedSet::has_table(u32 op) const {
  const DS_ID table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id) {
      return true;
    }
  }
  return false;
}

DS_ID FCFSCachedSet::add_table(u32 op) {
  Table new_table(build_table_name(id, op), capacity, keys_sizes, {});
  tables.push_back(new_table);
  return new_table.id;
}

void FCFSCachedSet::remove_table(const DS_ID &table_id) {
  tables.erase(std::remove_if(tables.begin(), tables.end(), [&table_id](const Table &table) { return table.id == table_id; }), tables.end());
}

const Table *FCFSCachedSet::get_table(u32 op) const {
  const DS_ID table_id = build_table_name(id, op);
  return get_table(table_id);
}

const Table *FCFSCachedSet::get_table(const DS_ID &table_id) const {
  for (const Table &table : tables) {
    if (table.id == table_id) {
      return &table;
    }
  }
  return nullptr;
}

bool FCFSCachedSet::has_hash(u32 op) const {
  const DS_ID hash_id = build_hash_name(id, op);
  for (const Hash &hash : hashes) {
    if (hash.id == hash_id) {
      return true;
    }
  }
  return false;
}

DS_ID FCFSCachedSet::add_hash(u32 op) {
  const bits_t hash_size = bits_from_pow2_capacity(capacity);
  Hash new_hash(build_hash_name(id, op), keys_sizes, hash_size);
  hashes.push_back(new_hash);
  return new_hash.id;
}

const Hash *FCFSCachedSet::get_hash(u32 op) const {
  const DS_ID hash_id = build_hash_name(id, op);
  return get_hash(hash_id);
}

const Hash *FCFSCachedSet::get_hash(const DS_ID &hash_id) const {
  for (const Hash &hash : hashes) {
    if (hash.id == hash_id) {
      return &hash;
    }
  }
  return nullptr;
}

void FCFSCachedSet::remove_hash(const DS_ID &hash_id) {
  hashes.erase(std::remove_if(hashes.begin(), hashes.end(), [&hash_id](const Hash &hash) { return hash.id == hash_id; }), hashes.end());
}

} // namespace Tofino
} // namespace LibSynapse