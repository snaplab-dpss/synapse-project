#include "meter.h"

#include "../../../exprs/retriever.h"

namespace tofino {

Meter::Meter(DS_ID _id, u32 _capacity, Bps_t _rate, bytes_t _burst,
             const std::vector<bits_t> &_keys)
    : DS(DSType::METER, true, _id), capacity(_capacity), rate(_rate),
      burst(_burst), keys(_keys) {
  ASSERT(capacity > 0, "Meter capacity must be greater than 0");
}

Meter::Meter(const Meter &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity),
      rate(other.rate), burst(other.burst), keys(other.keys) {}

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
  Log::dbg() << "\n";
  Log::dbg() << "=========== METER ============\n";
  Log::dbg() << "ID:        " << id << "\n";
  Log::dbg() << "Primitive: " << primitive << "\n";
  Log::dbg() << "Entries:   " << capacity << "\n";
  Log::dbg() << "Rate:      " << rate << " Bps\n";
  Log::dbg() << "Burst:     " << burst << " B\n";
  Log::dbg() << "Xbar:      " << get_match_xbar_consume() / 8 << " B\n";
  Log::dbg() << "SRAM:      " << get_consumed_sram() / 8 << " B\n";
  Log::dbg() << "==============================\n";
}

std::vector<klee::ref<klee::Expr>>
Meter::build_keys(klee::ref<klee::Expr> key) {
  std::vector<klee::ref<klee::Expr>> keys;

  std::vector<expr_group_t> groups = get_expr_groups(key);
  for (const expr_group_t &group : groups) {
    keys.push_back(group.expr);
  }

  return keys;
}

} // namespace tofino
