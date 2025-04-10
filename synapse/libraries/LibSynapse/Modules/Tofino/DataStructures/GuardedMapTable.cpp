#include <LibSynapse/Modules/Tofino/DataStructures/GuardedMapTable.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

namespace {

std::string build_table_name(DS_ID id, LibBDD::node_id_t table_num) { return id + "_" + std::to_string(table_num); }

Register build_guard(const tna_properties_t &properties, DS_ID id) {
  const DS_ID reg_id              = id + "_guard";
  const u32 capacity              = 1;
  const bits_t index_size         = 1;
  const bits_t value_size         = 8;
  const RegisterActionType action = RegisterActionType::Read;
  return Register(properties, reg_id, capacity, index_size, value_size, {action});
}

} // namespace

GuardedMapTable::GuardedMapTable(const tna_properties_t &properties, DS_ID _id, u32 _capacity, bits_t _key_size)
    : DS(DSType::GuardedMapTable, false, _id), capacity(_capacity), key_size(_key_size), param_size(32), guard(build_guard(properties, _id)) {
  assert(capacity > 0 && "Capacity must be greater than 0");
  assert(key_size > 0 && "Keys must be greater than 0");
}

GuardedMapTable::GuardedMapTable(const GuardedMapTable &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), key_size(other.key_size), param_size(other.param_size),
      tables(other.tables), guard(other.guard) {}

DS *GuardedMapTable::clone() const { return new GuardedMapTable(*this); }

void GuardedMapTable::debug() const {
  std::cerr << "\n";
  std::cerr << "======== GUARDED MAP TABLE ========\n";
  std::cerr << "ID:         " << id << "\n";
  std::cerr << "Entries:    " << capacity << "\n";
  std::cerr << "Key size:   " << key_size << "\n";
  std::cerr << "Param size: " << param_size << "\n";
  for (const Table &table : tables) {
    table.debug();
  }
  std::cerr << "===================================\n";
}

std::vector<std::unordered_set<const DS *>> GuardedMapTable::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();
  for (const Table &table : tables) {
    internal_ds.back().insert(&table);
  }
  internal_ds.back().insert(&guard);

  return internal_ds;
}

bool GuardedMapTable::has_table(LibBDD::node_id_t op) const {
  const std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return true;
  }
  return false;
}

const Table *GuardedMapTable::get_table(LibBDD::node_id_t op) const {
  const std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return &table;
  }
  return nullptr;
}

std::optional<DS_ID> GuardedMapTable::add_table(LibBDD::node_id_t op, const std::vector<bits_t> &keys_sizes, TimeAware time_aware) {
  Table new_table(build_table_name(id, op), capacity, keys_sizes, {param_size}, time_aware);
  tables.push_back(new_table);
  return new_table.id;
}

} // namespace Tofino
} // namespace LibSynapse