#include <LibSynapse/Modules/Tofino/DataStructures/CuckooHashTable.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

using LibCore::bits_from_pow2_capacity;

CuckooHashTable::CuckooHashTable(const tna_properties_t &properties, DS_ID _id, u32 _op, u32 _capacity)
    : DS(DSType::CuckooHashTable, false, _id), capacity(_capacity), cuckoo_index_size(bits_from_pow2_capacity(capacity)),
      cuckoo_bloom_index_size(bits_from_pow2_capacity(BLOOM_WIDTH)) {
  assert(capacity > 0);
}

CuckooHashTable::CuckooHashTable(const CuckooHashTable &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), cuckoo_index_size(other.cuckoo_index_size),
      cuckoo_bloom_index_size(other.cuckoo_bloom_index_size) {}

DS *CuckooHashTable::clone() const { return new CuckooHashTable(*this); }

void CuckooHashTable::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== Cuckoo Hash Table ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << capacity << "\n";
  std::cerr << "===========================================\n";
}

std::vector<std::unordered_set<const DS *>> CuckooHashTable::get_internal() const {
  std::vector<std::unordered_set<const DS *>> internal_ds;
  return internal_ds;
}

} // namespace Tofino
} // namespace LibSynapse