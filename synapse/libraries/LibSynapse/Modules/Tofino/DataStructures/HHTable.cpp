#include <LibSynapse/Modules/Tofino/DataStructures/HHTable.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

namespace {

std::string build_table_name(DS_ID id, u32 table_num) { return id + "_table_" + std::to_string(table_num); }

Register build_cached_counters(const tna_properties_t &properties, DS_ID id, u32 capacity) {
  const DS_ID reg_id              = id + "_cached_counters";
  const bits_t index_size         = 32;
  const bits_t value_size         = 32;
  const RegisterActionType action = RegisterActionType::INC;
  return Register(properties, reg_id, capacity, index_size, value_size, {action});
}

std::vector<Hash> build_hashes(DS_ID id, std::vector<bits_t> keys_sizes, u32 total_hashes, bits_t hash_size) {
  assert(!keys_sizes.empty() && "Keys sizes must not be empty");

  keys_sizes.push_back(32); // Add the hash salt

  std::vector<Hash> hashes;
  for (u32 i = 0; i < total_hashes; i++) {
    hashes.emplace_back(id + "_hash_calc_" + std::to_string(i), keys_sizes, hash_size);
  }

  return hashes;
}

std::vector<Register> build_count_min_sketch(const tna_properties_t &properties, DS_ID id, u32 cms_width, u32 cms_height, bits_t index_size) {
  const bits_t value_size         = 32;
  const RegisterActionType action = RegisterActionType::INC_AND_RETURN_NEW_VALUE;

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
  const RegisterActionType action = RegisterActionType::CALCULATE_DIFF;
  return Register(properties, reg_id, capacity, index_size, value_size, {action});
}

Digest build_digest(DS_ID id, const std::vector<bits_t> &fields, u8 digest_type) {
  const DS_ID digest_id = id + "_digest";
  return Digest(digest_id, fields, digest_type);
}

} // namespace

const std::vector<u32> HHTable::HASH_SALTS = {0xfbc31fc7, 0x2681580b, 0x486d7e2f, 0x1f3a2b4d, 0x7c5e9f8b, 0x3a2b4d1f,
                                              0x5e9f8b7c, 0x2b4d1f3a, 0x9f8b7c5e, 0xb4d1f3a2, 0x4d1f3a2b, 0x8b7c5e9f};

HHTable::HHTable(const tna_properties_t &properties, DS_ID _id, u32 _op, u32 _capacity, const std::vector<bits_t> &_keys_sizes, u32 _cms_width,
                 u32 _cms_height, u8 _digest_type)
    : DS(DSType::HH_TABLE, false, _id), capacity(_capacity), keys_sizes(_keys_sizes), cms_width(_cms_width), cms_height(_cms_height),
      hash_size(LibCore::bits_from_pow2_capacity(_cms_width)), cached_counters(build_cached_counters(properties, _id, _capacity)),
      hashes(build_hashes(_id, _keys_sizes, _cms_height, hash_size)),
      count_min_sketch(build_count_min_sketch(properties, _id, _cms_width, _cms_height, hash_size)), threshold(build_threshold(properties, _id)),
      digest(build_digest(_id, _keys_sizes, _digest_type)) {
  assert(_keys_sizes.size() > 0 && "HH Table keys sizes must not be empty");
  assert(capacity > 0 && "HH Table entries must be greater than 0");
  add_table(_op);
}

HHTable::HHTable(const HHTable &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), keys_sizes(other.keys_sizes), cms_width(other.cms_width),
      cms_height(other.cms_height), hash_size(other.hash_size), tables(other.tables), cached_counters(other.cached_counters), hashes(other.hashes),
      count_min_sketch(other.count_min_sketch), threshold(other.threshold), digest(other.digest) {}

DS *HHTable::clone() const { return new HHTable(*this); }

void HHTable::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== HH TABLE ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << capacity << "\n";
  std::cerr << "Hashes:    " << hashes.size() << " (" << hash_size << " bits)\n";
  std::cerr << "CMS:       " << cms_width << "x" << cms_height << "\n";
  std::cerr << "=================================\n";
}

std::vector<std::unordered_set<const DS *>> HHTable::get_internal() const {
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
  internal_ds.back().insert(&digest);

  return internal_ds;
}

bool HHTable::has_table(u32 op) const {
  const std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return true;
  }
  return false;
}

DS_ID HHTable::add_table(u32 op) {
  Table new_table(build_table_name(id, op), capacity, keys_sizes, {}, TimeAware::Yes);
  tables.push_back(new_table);
  return new_table.id;
}

const Table *HHTable::get_table(u32 op) const {
  const std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return &table;
  }
  return nullptr;
}

} // namespace Tofino
} // namespace LibSynapse