#pragma once

#include <LibSynapse/Target.h>
#include <LibSynapse/EPNode.h>
#include <LibBDD/BDD.h>
#include <LibCore/RandomEngine.h>
#include <LibCore/Types.h>

#include <unordered_map>
#include <unordered_set>

namespace LibSynapse {

using LibBDD::BDD;
using LibBDD::bdd_node_ids_t;
using LibBDD::BDDNode;
using LibCore::SingletonRandomEngine;

class EPLeaf;

struct EPMeta {
  size_t total_bdd_nodes;

  size_t depth;
  size_t nodes;
  size_t reordered_nodes;

  std::unordered_map<TargetArchitecture, size_t> steps_per_target;
  std::unordered_map<ModuleCategory, size_t> modules_counter;
  std::unordered_set<ep_node_id_t> processed_leaves;
  std::unordered_set<ep_node_id_t> visited_ep_nodes;
  bdd_node_ids_t processed_nodes;

  // Useful for uniquely identifying/classifying this EP.
  i64 random_number;

  EPMeta(const BDD *bdd, const TargetsView &targets)
      : total_bdd_nodes(bdd->size()), depth(0), nodes(0), reordered_nodes(0), random_number(SingletonRandomEngine::generate()) {
    for (const TargetView &target : targets.elements) {
      steps_per_target[target.type.type] = 0;
    }
  }

  EPMeta(const EPMeta &other)
      : total_bdd_nodes(other.total_bdd_nodes), depth(other.depth), nodes(other.nodes), reordered_nodes(other.reordered_nodes),
        steps_per_target(other.steps_per_target), modules_counter(other.modules_counter), processed_leaves(other.processed_leaves),
        visited_ep_nodes(other.visited_ep_nodes), processed_nodes(other.processed_nodes), random_number(SingletonRandomEngine::generate()) {}

  EPMeta(EPMeta &&other)
      : total_bdd_nodes(other.total_bdd_nodes), depth(other.depth), nodes(other.nodes), reordered_nodes(other.reordered_nodes),
        steps_per_target(std::move(other.steps_per_target)), modules_counter(std::move(other.modules_counter)),
        processed_leaves(std::move(other.processed_leaves)), visited_ep_nodes(std::move(other.visited_ep_nodes)),
        processed_nodes(std::move(other.processed_nodes)), random_number(SingletonRandomEngine::generate()) {}

  EPMeta &operator=(const EPMeta &other) {
    if (this == &other) {
      return *this;
    }

    depth            = other.depth;
    nodes            = other.nodes;
    reordered_nodes  = other.reordered_nodes;
    steps_per_target = other.steps_per_target;
    modules_counter  = other.modules_counter;
    processed_leaves = other.processed_leaves;
    visited_ep_nodes = other.visited_ep_nodes;
    processed_nodes  = other.processed_nodes;
    random_number    = SingletonRandomEngine::generate();

    return *this;
  }

  double get_bdd_progress() const;
  void process_node(const BDDNode *node, TargetType target);
  void update_total_bdd_nodes(const BDD *bdd);
  void update(const EPLeaf &leaf, const EPNode *new_node, bool process_node);
};

} // namespace LibSynapse
