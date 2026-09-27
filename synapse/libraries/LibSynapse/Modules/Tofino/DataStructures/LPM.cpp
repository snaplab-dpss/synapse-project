#include <LibSynapse/Modules/Tofino/DataStructures/LPM.h>
#include <LibCore/Expr.h>

#include <iostream>
#include <cassert>

namespace LibSynapse {
namespace Tofino {

LPM::LPM(DS_ID _id, u32 _capacity, const std::vector<bits_t> &_keys_size, bool _dns_name_key)
    : DS(DSType::LPM, true, _id), capacity(_capacity), key(0), keys_size(_keys_size), dns_name_key(_dns_name_key) {
  for (bits_t size : keys_size) {
    key += size;
  }
}

LPM::LPM(const LPM &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), key(other.key), keys_size(other.keys_size), dns_name_key(other.dns_name_key) {}

DS *LPM::clone() const { return new LPM(*this); }
bits_t LPM::get_match_xbar_consume() const { return key; }
bits_t LPM::get_consumed_tcam() const { return key * capacity; }

void LPM::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== LPM ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << capacity << "\n";
  std::cerr << "Key:       " << key << " b\n";
  std::cerr << "Xbar:      " << get_match_xbar_consume() / 8 << " B\n";
  std::cerr << "TCAM:      " << get_consumed_tcam() / 8 << " B\n";
  std::cerr << "==============================\n";
}

} // namespace Tofino
} // namespace LibSynapse