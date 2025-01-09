#include "table.h"

#include "../../../util/exprs.h"

namespace synapse {
namespace tofino {

Table::Table(DS_ID _id, u32 _num_entries, const std::vector<bits_t> &_keys,
             const std::vector<bits_t> &_params)
    : DS(DSType::TABLE, true, _id), num_entries(_num_entries), keys(_keys), params(_params) {
  assert(_num_entries > 0 && "Table entries must be greater than 0");
}

Table::Table(const Table &other)
    : DS(other.type, other.primitive, other.id), num_entries(other.num_entries), keys(other.keys),
      params(other.params) {}

DS *Table::clone() const { return new Table(*this); }

bits_t Table::get_match_xbar_consume() const {
  bits_t consumed = 0;
  for (bits_t key_size : keys)
    consumed += key_size;
  return consumed;
}

bits_t Table::get_consumed_sram() const {
  bits_t consumed = 0;
  for (bits_t key_size : keys)
    consumed += key_size;
  for (bits_t param_size : params)
    consumed += param_size;
  return consumed * num_entries;
}

void Table::debug() const {
  std::cerr << "\n";
  std::cerr << "=========== TABLE ============\n";
  std::cerr << "ID:        " << id << "\n";
  std::cerr << "Primitive: " << primitive << "\n";
  std::cerr << "Entries:   " << num_entries << "\n";
  std::cerr << "Xbar:      " << get_match_xbar_consume() / 8 << " B\n";
  std::cerr << "SRAM:      " << get_consumed_sram() / 8 << " B\n";
  std::cerr << "==============================\n";
}

std::vector<klee::ref<klee::Expr>> Table::build_keys(klee::ref<klee::Expr> key) {
  std::vector<klee::ref<klee::Expr>> keys;

  std::vector<expr_group_t> groups = get_expr_groups(key);
  for (const expr_group_t &group : groups) {
    keys.push_back(group.expr);
  }

  return keys;
}

} // namespace tofino
} // namespace synapse