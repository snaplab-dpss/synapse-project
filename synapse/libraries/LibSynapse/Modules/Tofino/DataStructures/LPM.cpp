#include <LibSynapse/Modules/Tofino/DataStructures/LPM.h>
#include <LibCore/Expr.h>

#include <iostream>
#include <cassert>

namespace LibSynapse {
namespace Tofino {

LPM::LPM(DS_ID _id, bits_t _capacity) : DS(DSType::LPM, true, _id), capacity(_capacity) {}

LPM::LPM(const LPM &other) : DS(other.type, other.primitive, other.id), capacity(other.capacity) {}

DS *LPM::clone() const { return new LPM(*this); }
bits_t LPM::get_match_xbar_consume() const { return key; }
bits_t LPM::get_consumed_tcam() const { return key * capacity; }

void LPM::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== LPM ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << capacity << "\n";
  std::cerr << "Xbar:      " << get_match_xbar_consume() / 8 << " B\n";
  std::cerr << "TCAM:      " << get_consumed_tcam() / 8 << " B\n";
  std::cerr << "==============================\n";
}

} // namespace Tofino
} // namespace LibSynapse