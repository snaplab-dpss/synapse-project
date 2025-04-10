#include <LibSynapse/Modules/Tofino/DataStructures/DchainTable.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

namespace {

std::string build_table_name(DS_ID id, LibBDD::node_id_t table_num) { return id + "_" + std::to_string(table_num); }

} // namespace

DchainTable::DchainTable(DS_ID _id, u32 _capacity) : DS(DSType::DchainTable, false, _id), capacity(_capacity), key_size(32) {
  assert(capacity > 0 && "Capacity must be greater than 0");
}

DchainTable::DchainTable(const DchainTable &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), key_size(other.key_size), tables(other.tables) {}

DS *DchainTable::clone() const { return new DchainTable(*this); }

void DchainTable::debug() const {
  std::cerr << "\n";
  std::cerr << "======== MAP TABLE ========\n";
  std::cerr << "ID:         " << id << "\n";
  std::cerr << "Entries:    " << capacity << "\n";
  std::cerr << "Key size:   " << key_size << "\n";
  for (const Table &table : tables) {
    table.debug();
  }
  std::cerr << "===========================\n";
}

std::vector<std::unordered_set<const DS *>> DchainTable::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();
  for (const Table &table : tables) {
    internal_ds.back().insert(&table);
  }

  return internal_ds;
}

bool DchainTable::has_table(LibBDD::node_id_t op) const {
  std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return true;
  }
  return false;
}

const Table *DchainTable::get_table(LibBDD::node_id_t op) const {
  std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return &table;
  }
  return nullptr;
}

std::optional<DS_ID> DchainTable::add_table(LibBDD::node_id_t op) {
  Table new_table(build_table_name(id, op), capacity, {key_size}, {}, TimeAware::Yes);
  tables.push_back(new_table);
  return new_table.id;
}

} // namespace Tofino
} // namespace LibSynapse