#pragma once

#include <unordered_map>
#include <unordered_set>

#include "../bdd/bdd.h"
#include "node.h"
#include "../targets/target.h"
#include "../profiler.h"
#include "../random_engine.h"

class EPLeaf;

struct EPMeta {
  size_t total_bdd_nodes;

  size_t depth;
  size_t nodes;
  size_t reordered_nodes;

  std::unordered_map<TargetType, size_t> nodes_per_target;
  std::unordered_map<TargetType, size_t> bdd_nodes_per_target;
  std::unordered_set<ep_node_id_t> visited_ep_nodes;
  nodes_t processed_nodes;

  // Useful for uniquely identifying/classifying this EP.
  int64_t random_number;

  EPMeta(const BDD *bdd, const std::unordered_set<TargetType> &targets,
         const TargetType initial_target)
      : total_bdd_nodes(bdd->size()), depth(0), nodes(0), reordered_nodes(0),
        random_number(RandomEngine::generate()) {
    for (TargetType target : targets) {
      nodes_per_target[target] = 0;
      bdd_nodes_per_target[target] = 0;
    }
  }

  EPMeta(const EPMeta &other)
      : total_bdd_nodes(other.total_bdd_nodes), depth(other.depth),
        nodes(other.nodes), reordered_nodes(other.reordered_nodes),
        nodes_per_target(other.nodes_per_target),
        bdd_nodes_per_target(other.bdd_nodes_per_target),
        processed_nodes(other.processed_nodes),
        random_number(RandomEngine::generate()) {}

  EPMeta(EPMeta &&other)
      : total_bdd_nodes(other.total_bdd_nodes), depth(other.depth),
        nodes(other.nodes), reordered_nodes(other.reordered_nodes),
        nodes_per_target(std::move(other.nodes_per_target)),
        bdd_nodes_per_target(std::move(other.bdd_nodes_per_target)),
        processed_nodes(std::move(other.processed_nodes)),
        random_number(RandomEngine::generate()) {}

  EPMeta &operator=(const EPMeta &other) {
    if (this == &other) {
      return *this;
    }

    depth = other.depth;
    nodes = other.nodes;
    reordered_nodes = other.reordered_nodes;
    nodes_per_target = other.nodes_per_target;
    bdd_nodes_per_target = other.bdd_nodes_per_target;
    processed_nodes = other.processed_nodes;
    random_number = RandomEngine::generate();

    return *this;
  }

  float get_bdd_progress() const;
  void process_node(const Node *node, TargetType target);
  void update_total_bdd_nodes(const BDD *bdd);
  void update(const EPLeaf *leaf, const EPNode *new_node, bool process_node);
};