#include <LibSynapse/Modules/Tofino/DataStructures/Hash.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

Hash::Hash(DS_ID _id, const std::vector<bits_t> &_keys, bits_t _size) : DS(DSType::HASH, true, _id), keys(_keys), size(_size) {}

Hash::Hash(const Hash &other) : DS(other.type, other.primitive, other.id), keys(other.keys), size(other.size) {}

DS *Hash::clone() const { return new Hash(*this); }

bits_t Hash::get_match_xbar_consume() const {
  bits_t consumed = 0;
  for (bits_t key_size : keys)
    consumed += key_size;
  return consumed;
}

void Hash::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== Hash ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Size:      " << size << " bits\n";
  std::cerr << "Xbar:      " << get_match_xbar_consume() / 8 << " B\n";
  std::cerr << "==============================\n";
}

} // namespace Tofino
} // namespace LibSynapse