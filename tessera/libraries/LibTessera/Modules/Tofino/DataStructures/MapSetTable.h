#pragma once

#include <LibBDD/Nodes/Node.h>
#include <LibTessera/Modules/Tofino/DataStructures/DataStructure.h>
#include <LibTessera/Modules/Tofino/DataStructures/Table.h>
#include <LibCore/Types.h>

#include <vector>

#include <klee/Expr.h>

namespace LibTessera {
namespace Tofino {

using LibBDD::bdd_node_id_t;

struct MapSetTable : public DS {
  u32 capacity;
  bits_t key_size;
  std::vector<Table> tables;

  MapSetTable(DS_ID id, u32 capacity, bits_t key_size);

  MapSetTable(const MapSetTable &other);

  DS *clone() const override;
  void debug() const override;
  std::vector<std::unordered_set<const DS *>> get_internal() const override;

  bool has_table(bdd_node_id_t op) const;
  const Table *get_table(bdd_node_id_t op) const;
  DS_ID add_table(bdd_node_id_t op, const std::vector<bits_t> &keys_sizes);
  void remove_table(bdd_node_id_t op);
};

} // namespace Tofino
} // namespace LibTessera