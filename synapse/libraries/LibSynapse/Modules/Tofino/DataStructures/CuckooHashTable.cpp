#include <LibSynapse/Modules/Tofino/DataStructures/CuckooHashTable.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

using LibCore::bits_from_pow2_capacity;

namespace {

std::array<Hash, 3> build_cuckoo_hashes(DS_ID id, bits_t key_size, bits_t hash_size) {
  const std::vector<bits_t> keys_sizes{key_size, 32}; // Add the key and the hash salt

  std::array<Hash, 3> hashes = {
      Hash(id + "_hash_calc_0", keys_sizes, hash_size),
      Hash(id + "_hash_calc_1", keys_sizes, hash_size),
      Hash(id + "_hash_calc_2", keys_sizes, hash_size),
  };

  return hashes;
}

std::array<Hash, 3> build_bloom_hashes(DS_ID id, bits_t key_size, bits_t hash_size) {
  const std::vector<bits_t> keys_sizes{key_size};

  std::array<Hash, 3> hashes = {
      Hash(id + "_hash_old_key", keys_sizes, hash_size),
      Hash(id + "_hash_new_key", keys_sizes, hash_size),
      Hash(id + "_hash_old_key_2", keys_sizes, hash_size),
  };

  return hashes;
}

} // namespace

CuckooHashTable::CuckooHashTable(const tna_properties_t &properties, DS_ID _id, u32 _op, u32 _capacity)
    : DS(DSType::CuckooHashTable, false, _id), total_capacity(_capacity), entries_per_cuckoo_table(_capacity / 2),
      cuckoo_index_size(bits_from_pow2_capacity(entries_per_cuckoo_table)), cuckoo_bloom_index_size(bits_from_pow2_capacity(BLOOM_WIDTH)),
      cuckoo_hashes(build_cuckoo_hashes(_id, SUPPORTED_KEY_SIZE, cuckoo_index_size)),
      reg_k_1(properties, _id + "_reg_k_1", entries_per_cuckoo_table, cuckoo_index_size, SUPPORTED_KEY_SIZE,
              {RegisterActionType::Read, RegisterActionType::Swap}),
      reg_k_2(properties, _id + "_reg_k_2", entries_per_cuckoo_table, cuckoo_index_size, SUPPORTED_KEY_SIZE,
              {RegisterActionType::Read, RegisterActionType::Swap}),
      reg_v_1(properties, _id + "_reg_v_1", entries_per_cuckoo_table, cuckoo_index_size, SUPPORTED_VALUE_SIZE,
              {RegisterActionType::ReadConditionalWrite, RegisterActionType::Swap}),
      reg_v_2(properties, _id + "_reg_v_2", entries_per_cuckoo_table, cuckoo_index_size, SUPPORTED_VALUE_SIZE,
              {RegisterActionType::ReadConditionalWrite, RegisterActionType::Swap}),
      reg_ts_1(properties, _id + "_reg_ts_1", entries_per_cuckoo_table, cuckoo_index_size, 32,
               {RegisterActionType::QueryAndRefreshTimestamp, RegisterActionType::Swap}),
      reg_ts_2(properties, _id + "_reg_ts_2", entries_per_cuckoo_table, cuckoo_index_size, 32,
               {RegisterActionType::QueryAndRefreshTimestamp, RegisterActionType::Swap}),
      bloom_hashes(build_bloom_hashes(_id, SUPPORTED_KEY_SIZE, cuckoo_bloom_index_size)),
      swap_transient(properties, _id + "_swap_transient", BLOOM_WIDTH, cuckoo_bloom_index_size, BLOOM_VALUE_SIZE,
                     {RegisterActionType::Read, RegisterActionType::Swap}),
      swapped_transient(properties, _id + "_swapped_transient", BLOOM_WIDTH, cuckoo_bloom_index_size, BLOOM_VALUE_SIZE,
                        {RegisterActionType::Read, RegisterActionType::Swap}) {
  assert(total_capacity > 0);
}

CuckooHashTable::CuckooHashTable(const CuckooHashTable &other)
    : DS(other.type, other.primitive, other.id), total_capacity(other.total_capacity), entries_per_cuckoo_table(other.entries_per_cuckoo_table),
      cuckoo_index_size(other.cuckoo_index_size), cuckoo_bloom_index_size(other.cuckoo_bloom_index_size), cuckoo_hashes(other.cuckoo_hashes),
      reg_k_1(other.reg_k_1), reg_k_2(other.reg_k_2), reg_v_1(other.reg_v_1), reg_v_2(other.reg_v_2), reg_ts_1(other.reg_ts_1),
      reg_ts_2(other.reg_ts_2), bloom_hashes(other.bloom_hashes), swap_transient(other.swap_transient), swapped_transient(other.swapped_transient) {}

DS *CuckooHashTable::clone() const { return new CuckooHashTable(*this); }

void CuckooHashTable::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== Cuckoo Hash Table ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << entries_per_cuckoo_table << "\n";
  std::cerr << "===========================================\n";
}

std::vector<std::unordered_set<const DS *>> CuckooHashTable::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  // This is not how it actually works. Most of the resources are spread out because of gateways conditions. However, we don't model those. So we're
  // forcing the resources to be spread out across the pipeline.

  internal_ds.emplace_back();
  internal_ds.back().insert(&cuckoo_hashes.at(0));

  internal_ds.emplace_back();
  internal_ds.back().insert(&cuckoo_hashes.at(1));

  internal_ds.emplace_back();
  internal_ds.back().insert(&cuckoo_hashes.at(2));

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_k_1);

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_k_2);

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_v_1);

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_v_2);

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_ts_1);

  internal_ds.emplace_back();
  internal_ds.back().insert(&reg_ts_2);

  internal_ds.emplace_back();
  internal_ds.back().insert(&bloom_hashes.at(0));

  internal_ds.emplace_back();
  internal_ds.back().insert(&bloom_hashes.at(1));

  internal_ds.emplace_back();
  internal_ds.back().insert(&bloom_hashes.at(2));

  internal_ds.emplace_back();
  internal_ds.back().insert(&swap_transient);

  internal_ds.emplace_back();
  internal_ds.back().insert(&swapped_transient);

  return internal_ds;
}

} // namespace Tofino
} // namespace LibSynapse