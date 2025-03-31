#include <LibSynapse/Modules/Tofino/DataStructures/MapTable.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

namespace {

std::string build_table_name(DS_ID id, LibBDD::node_id_t table_num) { return id + "_" + std::to_string(table_num); }

} // namespace

MapTable::MapTable(DS_ID _id, u32 _capacity, bits_t _key_size)
    : DS(DSType::MAP_TABLE, false, _id), capacity(_capacity), key_size(_key_size), param_size(32) {
  assert(capacity > 0 && "Capacity must be greater than 0");
  assert(key_size > 0 && "Keys must be greater than 0");
}

MapTable::MapTable(const MapTable &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), key_size(other.key_size), param_size(other.param_size),
      tables(other.tables) {}

DS *MapTable::clone() const { return new MapTable(*this); }

void MapTable::debug() const {
  std::cerr << "\n";
  std::cerr << "======== MAP TABLE ========\n";
  std::cerr << "ID:         " << id << "\n";
  std::cerr << "Entries:    " << capacity << "\n";
  std::cerr << "Key size:   " << key_size << "\n";
  std::cerr << "Param size: " << param_size << "\n";
  for (const Table &table : tables) {
    table.debug();
  }
  std::cerr << "===========================\n";
}

std::vector<std::unordered_set<const DS *>> MapTable::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();
  for (const Table &table : tables) {
    internal_ds.back().insert(&table);
  }

  return internal_ds;
}

bool MapTable::has_table(LibBDD::node_id_t op) const {
  const std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return true;
  }
  return false;
}

const Table *MapTable::get_table(LibBDD::node_id_t op) const {
  const std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return &table;
  }
  return nullptr;
}

std::optional<DS_ID> MapTable::add_table(LibBDD::node_id_t op, const std::vector<bits_t> &keys_sizes, TimeAware time_aware) {
  Table new_table(build_table_name(id, op), capacity, keys_sizes, {param_size}, time_aware);
  tables.push_back(new_table);
  return new_table.id;
}

} // namespace Tofino
} // namespace LibSynapse