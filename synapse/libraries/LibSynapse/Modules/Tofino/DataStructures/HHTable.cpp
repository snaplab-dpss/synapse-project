#include <LibSynapse/Modules/Tofino/DataStructures/HHTable.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

namespace {
std::string build_table_name(DS_ID id, u32 table_num) { return id + "_table_" + std::to_string(table_num); }

std::string build_cms_name(DS_ID id) { return id + "_cms"; }
} // namespace

HHTable::HHTable(const TNAProperties &properties, DS_ID _id, u32 _op, u32 _num_entries, const std::vector<bits_t> &_keys_sizes,
                 u32 _cms_width, u32 _cms_height)
    : DS(DSType::HH_TABLE, false, _id), num_entries(_num_entries), keys_sizes(_keys_sizes), cms_width(_cms_width), cms_height(_cms_height),
      cms(properties, build_cms_name(id), keys_sizes, cms_width, cms_height) {
  assert(num_entries > 0 && "HH Table entries must be greater than 0");
  add_table(_op);
}

HHTable::HHTable(const HHTable &other)
    : DS(other.type, other.primitive, other.id), num_entries(other.num_entries), keys_sizes(other.keys_sizes), tables(other.tables),
      cms_width(other.cms_width), cms_height(other.cms_height), cms(other.cms) {}

DS *HHTable::clone() const { return new HHTable(*this); }

void HHTable::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== HH TABLE ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << num_entries << "\n";
  std::cerr << "CMS:       " << cms_width << "x" << cms_height << "\n";
  std::cerr << "=================================\n";
}

std::vector<std::unordered_set<const DS *>> HHTable::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;

  internal_ds.emplace_back();

  internal_ds.back().insert(&cms);
  for (const Table &table : tables) {
    internal_ds.back().insert(&table);
  }

  return internal_ds;
}

bool HHTable::has_table(u32 op) const {
  std::string table_id = build_table_name(id, op);
  for (const Table &table : tables) {
    if (table.id == table_id)
      return true;
  }
  return false;
}

DS_ID HHTable::add_table(u32 op) {
  Table new_table(build_table_name(id, op), num_entries, keys_sizes, {});
  tables.push_back(new_table);
  return new_table.id;
}

} // namespace Tofino
} // namespace LibSynapse