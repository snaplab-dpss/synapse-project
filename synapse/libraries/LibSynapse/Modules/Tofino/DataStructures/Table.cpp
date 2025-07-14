#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibSynapse/Modules/Tofino/TNA/TNA.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>
#include <LibCore/Debug.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

using LibCore::break_expr_into_structs_aware_chunks;

Table::Table(DS_ID _id, u32 _capacity, const std::vector<bits_t> &_keys, const std::vector<bits_t> &_params, TimeAware _time_aware)
    : DS(DSType::Table, true, _id), capacity(adjust_capacity_for_collisions(_capacity)), keys(_keys), params(_params), time_aware(_time_aware) {
  assert(_capacity > 0 && "Table entries must be greater than 0");
}

Table::Table(const Table &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), keys(other.keys), params(other.params), time_aware(other.time_aware) {}

DS *Table::clone() const { return new Table(*this); }

bits_t Table::get_match_xbar_consume() const {
  bits_t consumed = 0;
  for (bits_t key_size : keys)
    consumed += key_size;
  return consumed;
}

bits_t Table::get_consumed_sram() const { return get_consumed_sram_per_entry() * capacity; }

bits_t Table::get_consumed_sram_per_entry() const {
  bits_t consumed = 0;
  for (bits_t key_size : keys)
    consumed += key_size;
  for (bits_t param_size : params)
    consumed += param_size;
  return consumed;
}

void Table::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== TABLE ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << capacity << "\n";
  std::cerr << "Keys:      [";
  for (bits_t key_size : keys)
    std::cerr << key_size << " ";
  std::cerr << "]\n";
  std::cerr << "Params:    [";
  for (bits_t param_size : params)
    std::cerr << param_size << " ";
  std::cerr << "]\n";
  std::cerr << "Xbar:      " << get_match_xbar_consume() / 8 << " B\n";
  std::cerr << "SRAM:      " << get_consumed_sram() / 8 << " B\n";
  std::cerr << "Timed:     " << ((time_aware == TimeAware::Yes) ? "yes" : "no") << "\n";
  std::cerr << "==============================\n";
}

u32 Table::adjust_capacity_for_collisions(u32 capacity) { return std::ceil(capacity / TNA::TABLE_CAPACITY_EFFICIENCY); }

std::vector<klee::ref<klee::Expr>> Table::build_keys(klee::ref<klee::Expr> key, const std::vector<expr_struct_t> &headers) {
  const std::vector<klee::ref<klee::Expr>> &keys = break_expr_into_structs_aware_chunks(key, headers);
  return keys;
}

} // namespace Tofino
} // namespace LibSynapse