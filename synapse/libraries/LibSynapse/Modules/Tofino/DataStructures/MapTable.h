#pragma once

#include <LibBDD/Nodes/Node.h>
#include <LibSynapse/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibCore/Types.h>

#include <vector>

#include <klee/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::bdd_node_id_t;

struct MapTable : public DS {
  u32 capacity;
  bits_t key_size;
  bits_t param_size;
  std::vector<Table> tables;

  MapTable(DS_ID id, u32 capacity, bits_t key_size);

  MapTable(const MapTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;

  bits_t get_match_xbar_consume() const;
  bits_t get_consumed_sram() const;

  bool has_table(bdd_node_id_t op) const;
  const Table *get_table(bdd_node_id_t op) const;
  std::optional<DS_ID> add_table(bdd_node_id_t op, const std::vector<bits_t> &keys_sizes, TimeAware time_aware);
};

} // namespace Tofino
} // namespace LibSynapse