#include "hh_table.h"

#include "../../../exprs/retriever.h"

namespace tofino {

HHTable::HHTable(DS_ID _id, int op, int _num_entries, int _cms_width,
                 int _cms_height, const std::vector<bits_t> &_keys)
    : DS(DSType::HH_TABLE, false, _id), num_entries(_num_entries),
      cms_width(_cms_width), cms_height(_cms_height) {
  assert(_num_entries > 0);
}

HHTable::HHTable(const HHTable &other)
    : DS(other.type, other.primitive, other.id), num_entries(other.num_entries),
      cms_width(other.cms_width), cms_height(other.cms_height) {}

DS *HHTable::clone() const { return new HHTable(*this); }

void HHTable::debug() const {
  Log::dbg() << "\n";
  Log::dbg() << "=========== HH TABLE ============\n";
  Log::dbg() << "ID:        " << id << "\n";
  Log::dbg() << "Primitive: " << primitive << "\n";
  Log::dbg() << "Entries:   " << num_entries << "\n";
  Log::dbg() << "CMS:       " << cms_width << "x" << cms_height << "\n";
  Log::dbg() << "=================================\n";
}

std::vector<std::unordered_set<const DS *>> HHTable::get_internal() const {
  return {};
}

} // namespace tofino
