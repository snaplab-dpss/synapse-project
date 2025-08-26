#include <LibSynapse/Modules/Tofino/DataStructures/CuckooHashTable.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

using LibCore::bits_from_pow2_capacity;

namespace {

std::string build_table_name(DS_ID id, u32 table_num) { return id + "_table_" + std::to_string(table_num); }

Register build_cached_counters(const tna_properties_t &properties, DS_ID id, u32 capacity) {
  const DS_ID reg_id              = id + "_cached_counters";
  const bits_t index_size         = 32;
  const bits_t value_size         = 32;
  const RegisterActionType action = RegisterActionType::Increment;
  return Register(properties, reg_id, capacity, index_size, value_size, {action});
}

std::vector<Hash> build_hashes(DS_ID id, bits_t key_size, u32 total_hashes, bits_t hash_size) {
  const std::vector<bits_t> hash_inputs_sizes{
      key_size,
      32, // Add the hash salt
  };

  std::vector<Hash> hashes;
  for (u32 i = 0; i < total_hashes; i++) {
    hashes.emplace_back(id + "_hash_calc_" + std::to_string(i), hash_inputs_sizes, hash_size);
  }

  return hashes;
}

std::vector<Register> build_count_min_sketch(const tna_properties_t &properties, DS_ID id, u32 cms_width, u32 cms_height, bits_t index_size) {
  const bits_t value_size         = 32;
  const RegisterActionType action = RegisterActionType::IncrementAndReturnNewValue;

  std::vector<Register> count_min_sketch;
  for (u32 i = 0; i < cms_height; i++) {
    const DS_ID row_id = id + "_cms_row_" + std::to_string(i);
    Register row(properties, row_id, cms_width, index_size, value_size, {action});
    count_min_sketch.push_back(row);
  }

  return count_min_sketch;
}

Register build_threshold(const tna_properties_t &properties, DS_ID id) {
  const DS_ID reg_id              = id + "_threshold";
  const u32 capacity              = 1;
  const bits_t index_size         = 1;
  const bits_t value_size         = 32;
  const RegisterActionType action = RegisterActionType::CalculateDiff;
  return Register(properties, reg_id, capacity, index_size, value_size, {action});
}

} // namespace

const std::vector<u32> CuckooHashTable::HASH_SALTS = {0xfbc31fc7, 0x2681580b, 0x486d7e2f, 0x1f3a2b4d, 0x7c5e9f8b, 0x3a2b4d1f,
                                                      0x5e9f8b7c, 0x2b4d1f3a, 0x9f8b7c5e, 0xb4d1f3a2, 0x4d1f3a2b, 0x8b7c5e9f};

CuckooHashTable::CuckooHashTable(const tna_properties_t &properties, DS_ID _id, u32 _op, u32 _capacity)
    : DS(DSType::CuckooHashTable, false, _id), capacity(_capacity), hash_size(bits_from_pow2_capacity(CMS_WIDTH)),
      cached_counters(build_cached_counters(properties, _id, _capacity)), hashes(build_hashes(_id, SUPPORTED_KEY_SIZE, CMS_HEIGHT, hash_size)),
      count_min_sketch(build_count_min_sketch(properties, _id, CMS_WIDTH, CMS_HEIGHT, hash_size)), threshold(build_threshold(properties, _id)) {
  assert(capacity > 0 && "HH Table entries must be greater than 0");
  add_table(_op);
}

CuckooHashTable::CuckooHashTable(const CuckooHashTable &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), hash_size(other.hash_size), tables(other.tables),
      cached_counters(other.cached_counters), hashes(other.hashes), count_min_sketch(other.count_min_sketch), threshold(other.threshold) {}

DS *CuckooHashTable::clone() const { return new CuckooHashTable(*this); }

void CuckooHashTable::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== Cuckoo Hash Table ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << capacity << "\n";
  std::cerr << "Hashes:    " << hashes.size() << " (" << hash_size << " bits)\n";
  std::cerr << "===========================================\n";
}

std::vector<std::unordered_set<const DS *>> CuckooHashTable::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();
  for (const Table &table : tables) {
    internal_ds.back().insert(&table);
  }

  internal_ds.emplace_back();
  internal_ds.back().insert(&cached_counters);
  for (const Hash &hash : hashes) {
    internal_ds.back().insert(&hash);
  }
  for (const Register &row : count_min_sketch) {
    internal_ds.back().insert(&row);
  }
  internal_ds.back().insert(&threshold);

  return internal_ds;
}

bool CuckooHashTable::has_table(u32 op) const {
  const std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return true;
  }
  return false;
}

DS_ID CuckooHashTable::add_table(u32 op) {
  Table new_table(build_table_name(id, op), capacity, {SUPPORTED_KEY_SIZE}, {}, TimeAware::Yes);
  tables.push_back(new_table);
  return new_table.id;
}

const Table *CuckooHashTable::get_table(u32 op) const {
  const std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return &table;
  }
  return nullptr;
}

} // namespace Tofino
} // namespace LibSynapse