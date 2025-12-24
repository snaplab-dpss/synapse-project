#include <LibSynapse/Modules/Tofino/DataStructures/FCFSCachedTable.h>

#include <cmath>
#include <iostream>

namespace LibSynapse {
namespace Tofino {

using LibCore::bits_from_pow2_capacity;

namespace {

DS_ID build_table_name(DS_ID id, u32 table_num) { return id + "_table_" + std::to_string(table_num); }

Register build_reg_liveness(const tna_properties_t &properties, DS_ID id, u32 cache_capacity) {
  const bits_t hash_size  = bits_from_pow2_capacity(cache_capacity);
  const bits_t match_size = 8;
  return Register(properties, id + "_reg_liveness", cache_capacity, hash_size, match_size,
                  {RegisterActionType::Read, RegisterActionType::SetToOneAndReturnOldValue});
}

Register build_reg_integer_allocator_head(const tna_properties_t &properties, DS_ID id, u32 capacity) {
  const bits_t index_size = bits_from_pow2_capacity(capacity);
  return Register(properties, id + "_reg_integer_allocator_head", capacity, index_size, index_size,
                  {RegisterActionType::IntegerAllocatorHeadReadAndUpdate});
}

Register build_reg_integer_allocator_tail(const tna_properties_t &properties, DS_ID id, u32 capacity) {
  const bits_t index_size = bits_from_pow2_capacity(capacity);
  return Register(properties, id + "_reg_integer_allocator_tail", capacity, index_size, index_size, {RegisterActionType::Read});
}

Register build_reg_integer_allocator_indexes(const tna_properties_t &properties, DS_ID id, u32 capacity) {
  const bits_t index_size = bits_from_pow2_capacity(capacity);
  return Register(properties, id + "_reg_integer_allocator_indexes", capacity, index_size, index_size, {RegisterActionType::Read});
}

Register build_reg_integer_allocator_pending(const tna_properties_t &properties, DS_ID id, u32 capacity) {
  const bits_t index_size = bits_from_pow2_capacity(capacity);
  const bits_t match_size = 8;
  return Register(properties, id + "_reg_integer_allocator_pending", capacity, index_size, match_size,
                  {RegisterActionType::Read, RegisterActionType::Write});
}

std::vector<Register> build_cache_keys(const tna_properties_t &properties, DS_ID id, const std::vector<bits_t> &elements_sizes, u32 capacity) {
  std::vector<Register> registers;

  const bits_t hash_size = bits_from_pow2_capacity(capacity);

  int i = 0;
  for (bits_t key_size : elements_sizes) {
    Register cache_key(properties, id + "_reg_key_" + std::to_string(i), capacity, hash_size, key_size,
                       {RegisterActionType::Read, RegisterActionType::Swap});
    i++;
    registers.push_back(cache_key);
  }

  return registers;
}

Register build_cache_value(const tna_properties_t &properties, DS_ID id, u32 capacity) {
  const bits_t hash_size  = bits_from_pow2_capacity(capacity);
  const bits_t value_size = 32;
  return Register(properties, id + "_reg_value", capacity, hash_size, value_size, {RegisterActionType::Read, RegisterActionType::Swap});
}

Hash build_hash(DS_ID id, const std::vector<bits_t> &keys_sizes, u32 capacity) {
  const bits_t hash_size = bits_from_pow2_capacity(capacity);
  return Hash(id + "_hash", keys_sizes, hash_size);
}

Digest build_digest(DS_ID id, const std::vector<bits_t> &fields, u8 digest_type) {
  const DS_ID digest_id = id + "_digest";
  return Digest(digest_id, fields, digest_type);
}

} // namespace

FCFSCachedTable::FCFSCachedTable(const tna_properties_t &properties, DS_ID _id, u32 _op, u32 _cache_capacity, u32 _capacity,
                                 const std::vector<bits_t> &_keys_sizes, u8 digest_type)
    : DS(DSType::FCFSCachedTable, false, _id), cache_capacity(_cache_capacity), capacity(_capacity), keys_sizes(_keys_sizes),
      reg_liveness(build_reg_liveness(properties, id, cache_capacity)),
      reg_integer_allocator_head(build_reg_integer_allocator_head(properties, id, capacity)),
      reg_integer_allocator_tail(build_reg_integer_allocator_tail(properties, id, capacity)),
      reg_integer_allocator_indexes(build_reg_integer_allocator_indexes(properties, id, capacity)),
      reg_integer_allocator_pending(build_reg_integer_allocator_pending(properties, id, capacity)),
      cache_keys(build_cache_keys(properties, id, keys_sizes, cache_capacity)), cache_value(build_cache_value(properties, id, cache_capacity)),
      hash(build_hash(_id, _keys_sizes, _capacity)), digest(build_digest(_id, _keys_sizes, digest_type)) {
  assert(cache_capacity > 0 && "Cache capacity must be greater than 0");
  assert(capacity > 0 && "Number of entries must be greater than 0");
  assert(cache_capacity <= capacity && "Cache capacity must be less than the "
                                       "number of entries");
  add_table(_op);
}

FCFSCachedTable::FCFSCachedTable(const FCFSCachedTable &other)
    : DS(other.type, other.primitive, other.id), cache_capacity(other.cache_capacity), capacity(other.capacity), keys_sizes(other.keys_sizes),
      tables(other.tables), reg_liveness(other.reg_liveness), reg_integer_allocator_head(other.reg_integer_allocator_head),
      reg_integer_allocator_tail(other.reg_integer_allocator_tail), reg_integer_allocator_indexes(other.reg_integer_allocator_indexes),
      reg_integer_allocator_pending(other.reg_integer_allocator_pending), cache_keys(other.cache_keys), cache_value(other.cache_value),
      hash(other.hash), digest(other.digest) {}

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
  reg_liveness.debug();
  reg_integer_allocator_head.debug();
  reg_integer_allocator_tail.debug();
  reg_integer_allocator_indexes.debug();
  reg_integer_allocator_pending.debug();
  for (const Register &cache_key : cache_keys) {
    cache_key.debug();
  }
  cache_value.debug();
  hash.debug();
  digest.debug();
  std::cerr << "==============================\n";
}

std::vector<std::unordered_set<const DS *>> FCFSCachedTable::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();
  for (const Table &table : tables)
    internal_ds.back().insert(&table);

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_liveness);

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_integer_allocator_tail);

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_integer_allocator_head);

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_integer_allocator_indexes);
  internal_ds.back().insert(&reg_integer_allocator_pending);

  internal_ds.emplace_back();
  for (const Register &cache_key : cache_keys)
    internal_ds.back().insert(&cache_key);
  internal_ds.back().insert(&digest);

  return internal_ds;
}

bool FCFSCachedTable::has_table(u32 op) const {
  const DS_ID table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id) {
      return true;
    }
  }
  return false;
}

std::optional<DS_ID> FCFSCachedTable::add_table(u32 op) {
  Table new_table(build_table_name(id, op), capacity - cache_capacity, keys_sizes, {});
  tables.push_back(new_table);
  return new_table.id;
}

void FCFSCachedTable::remove_table(const DS_ID &table_id) {
  tables.erase(std::remove_if(tables.begin(), tables.end(), [&table_id](const Table &table) { return table.id == table_id; }), tables.end());
}

const Table *FCFSCachedTable::get_table(u32 op) const {
  const DS_ID table_id = build_table_name(id, op);
  return get_table(table_id);
}

const Table *FCFSCachedTable::get_table(const DS_ID &table_id) const {
  for (const Table &table : tables) {
    if (table.id == table_id) {
      return &table;
    }
  }
  return nullptr;
}

} // namespace Tofino
} // namespace LibSynapse