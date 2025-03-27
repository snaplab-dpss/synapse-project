#include <LibSynapse/Modules/Tofino/DataStructures/Meter.h>
#include <LibCore/Expr.h>

#include <iostream>
#include <cassert>

namespace LibSynapse {
namespace Tofino {

Meter::Meter(DS_ID _id, u32 _capacity, Bps_t _rate, bytes_t _burst, const std::vector<bits_t> &_keys)
    : DS(DSType::METER, true, _id), capacity(_capacity), rate(_rate), burst(_burst), keys(_keys) {
  assert(capacity > 0 && "Meter capacity must be greater than 0");
}

Meter::Meter(const Meter &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), rate(other.rate), burst(other.burst), keys(other.keys) {}

DS *Meter::clone() const { return new Meter(*this); }

bits_t Meter::get_match_xbar_consume() const {
  bits_t consumed = 0;
  for (bits_t key_size : keys)
    consumed += key_size;
  return consumed;
}

bits_t Meter::get_consumed_sram() const {
  bits_t consumed = 0;
  for (bits_t key_size : keys)
    consumed += key_size;
  return consumed * capacity;
}

void Meter::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== METER ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << capacity << "\n";
  std::cerr << "Rate:      " << rate << " Bps\n";
  std::cerr << "Burst:     " << burst << " B\n";
  std::cerr << "Xbar:      " << get_match_xbar_consume() / 8 << " B\n";
  std::cerr << "SRAM:      " << get_consumed_sram() / 8 << " B\n";
  std::cerr << "==============================\n";
}

} // namespace Tofino
} // namespace LibSynapse