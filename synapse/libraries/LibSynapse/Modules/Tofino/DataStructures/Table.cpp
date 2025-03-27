#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>
#include <LibCore/Debug.h>

#include <iostream>

namespace LibSynapse {
namespace Tofino {

Table::Table(DS_ID _id, u32 _capacity, const std::vector<bits_t> &_keys, const std::vector<bits_t> &_params, TimeAware _time_aware)
    : DS(DSType::TABLE, true, _id), capacity(_capacity), keys(_keys), params(_params), time_aware(_time_aware) {
  assert(_capacity > 0 && "Table entries must be greater than 0");
}

Table::Table(const Table &other)
    : DS(other.type, other.primitive, other.id), capacity(other.capacity), keys(other.keys), params(other.params),
      time_aware(other.time_aware) {}

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
  return consumed * capacity;
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

std::vector<klee::ref<klee::Expr>> Table::build_keys(klee::ref<klee::Expr> key, const std::vector<LibBDD::header_t> &headers) {
  std::vector<klee::ref<klee::Expr>> keys;

  for (const LibCore::expr_group_t &group : LibCore::get_expr_groups(key)) {
    std::vector<klee::ref<klee::Expr>> subgroups{group.expr};

    bool split = true;
    while (split) {
      split = false;

      std::vector<klee::ref<klee::Expr>> subgroups_copy = subgroups;
      subgroups.clear();

      for (klee::ref<klee::Expr> expr : subgroups_copy) {
        for (const LibBDD::header_t &header : headers) {
          for (klee::ref<klee::Expr> field : header.fields) {
            LibCore::expr_pos_t pos;
            if (LibCore::is_smaller_expr_contained_in_expr(field, expr, pos)) {
              const std::vector<klee::ref<klee::Expr>> slices = LibCore::split_expr(expr, pos);
              subgroups.insert(subgroups.end(), slices.begin(), slices.end());
              split = true;
              break;
            }
          }

          if (split) {
            break;
          }
        }

        if (!split) {
          subgroups.push_back(expr);
        }
      }
    }

    const bytes_t max_group_size = 4;

    for (klee::ref<klee::Expr> subgroup : subgroups) {
      const bytes_t total_size = subgroup->getWidth() / 8;

      bytes_t processed_bytes = 0;
      while (processed_bytes != total_size) {
        const bytes_t target_size = std::min(max_group_size, total_size - processed_bytes);
        const bytes_t offset      = total_size - processed_bytes - target_size;

        klee::ref<klee::Expr> subsubgroup = LibCore::solver_toolbox.exprBuilder->Extract(subgroup, offset * 8, target_size * 8);
        subsubgroup                       = LibCore::simplify(subsubgroup);

        keys.push_back(subsubgroup);
        processed_bytes += target_size;
      }
    }
  }

  return keys;
}

} // namespace Tofino
} // namespace LibSynapse