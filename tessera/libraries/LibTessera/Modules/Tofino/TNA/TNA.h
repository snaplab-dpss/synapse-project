#pragma once

#include <LibTessera/Modules/Tofino/DataStructures/DataStructures.h>
#include <LibTessera/Modules/Tofino/TNA/TNAProperties.h>
#include <LibTessera/Modules/Tofino/TNA/Parser.h>
#include <LibTessera/Modules/Tofino/TNA/Pipeline.h>
#include <LibBDD/BDD.h>
#include <LibCore/Types.h>

#include <unordered_map>

namespace LibTessera {
namespace Tofino {

using LibBDD::BDD;
using LibBDD::BDDNode;
using LibCore::symbol_t;

struct TNA {
  // Actually, we usually only get around 90% of usage from the dataplane tables.
  // Higher than that and we start getting collisions, and errors trying to insert new entries.
  constexpr const static double TABLE_CAPACITY_EFFICIENCY{0.9};
  const tna_config_t tna_config;

  Parser parser;
  Pipeline pipeline;

  TNA(const tna_config_t &_tna_config, const DataStructures &_data_structures)
      : tna_config(_tna_config), parser(), pipeline(tna_config.properties, _data_structures) {}

  TNA(const TNA &other, const DataStructures &data_structures)
      : tna_config(other.tna_config), parser(other.parser), pipeline(other.pipeline, data_structures) {}

  // Tofino compiler complains if we access more than 4 bytes of the packet on the same if statement.
  bool condition_meets_phv_limit(klee::ref<klee::Expr> expr) const;

  std::vector<tofino_port_t> plausible_ingress_ports_in_bdd_node(const BDD *bdd, const BDDNode *node) const;

  bool is_simple_conditional_expr(klee::ref<klee::Expr> expr) const;
  bool is_simple_register_conditional_expr(const std::vector<klee::ref<klee::Expr>> &exprs, const symbol_t &register_value) const;

  void debug() const;
};

} // namespace Tofino
} // namespace LibTessera