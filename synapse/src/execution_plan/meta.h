#pragma once

#include <unordered_map>
#include <unordered_set>

#include "../bdd/bdd.h"
#include "node.h"
#include "../targets/target.h"
#include "../profiler.h"
#include "../random_engine.h"

namespace synapse {
class EPLeaf;

struct EPMeta {
  size_t total_bdd_nodes;

  size_t depth;
  size_t nodes;
  size_t reordered_nodes;

  std::unordered_map<TargetType, size_t> steps_per_target;
  std::unordered_set<ep_node_id_t> processed_leaves;
  std::unordered_set<ep_node_id_t> visited_ep_nodes;
  nodes_t processed_nodes;

  // Useful for uniquely identifying/classifying this EP.
  i64 random_number;

  EPMeta(const BDD *bdd, const TargetsView &targets)
      : total_bdd_nodes(bdd->size()), depth(0), nodes(0), reordered_nodes(0),
        random_number(RandomEngine::generate()) {
    for (const TargetView &target : targets.elements) {
      steps_per_target[target.type] = 0;
    }
  }

  EPMeta(const EPMeta &other)
      : total_bdd_nodes(other.total_bdd_nodes), depth(other.depth), nodes(other.nodes),
        reordered_nodes(other.reordered_nodes), steps_per_target(other.steps_per_target),
        processed_leaves(other.processed_leaves), visited_ep_nodes(other.visited_ep_nodes),
        processed_nodes(other.processed_nodes), random_number(RandomEngine::generate()) {}

  EPMeta(EPMeta &&other)
      : total_bdd_nodes(other.total_bdd_nodes), depth(other.depth), nodes(other.nodes),
        reordered_nodes(other.reordered_nodes), steps_per_target(std::move(other.steps_per_target)),
        processed_leaves(std::move(other.processed_leaves)),
        visited_ep_nodes(std::move(other.visited_ep_nodes)),
        processed_nodes(std::move(other.processed_nodes)), random_number(RandomEngine::generate()) {
  }

  EPMeta &operator=(const EPMeta &other) {
    if (this == &other) {
      return *this;
    }

    depth = other.depth;
    nodes = other.nodes;
    reordered_nodes = other.reordered_nodes;
    steps_per_target = other.steps_per_target;
    processed_leaves = other.processed_leaves;
    visited_ep_nodes = other.visited_ep_nodes;
    processed_nodes = other.processed_nodes;
    random_number = RandomEngine::generate();

    return *this;
  }

  double get_bdd_progress() const;
  void process_node(const Node *node, TargetType target);
  void update_total_bdd_nodes(const BDD *bdd);
  void update(const EPLeaf &leaf, const EPNode *new_node, bool process_node);
};
} // namespace synapse