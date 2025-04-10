#include <LibSynapse/Modules/Tofino/DataStructures/Digest.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

Digest::Digest(DS_ID _id, const std::vector<bits_t> &_fields, u8 _digest_type)
    : DS(DSType::Digest, true, _id), fields(_fields), digest_type(_digest_type) {}

Digest::Digest(const Digest &other) : DS(other.type, other.primitive, other.id), fields(other.fields), digest_type(other.digest_type) {}

DS *Digest::clone() const { return new Digest(*this); }

bits_t Digest::get_match_xbar_consume() const { return 0; }

void Digest::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== Digest ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Type:      " << digest_type << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Xbar:      " << get_match_xbar_consume() / 8 << " B\n";
  std::cerr << "==============================\n";
}

} // namespace Tofino
} // namespace LibSynapse