#pragma once

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <toml++/toml.hpp>

#include "node.h"
#include "meta.h"
#include "context.h"
#include "../bdd/bdd.h"
#include "../targets/target.h"

class EPVisitor;
class Profiler;
struct spec_impl_t;

typedef std::unordered_map<node_id_t, node_id_t> translator_t;

struct EPLeaf {
  EPNode *node;
  const Node *next;

  EPLeaf(EPNode *_node, const Node *_next) : node(_node), next(_next) {}
  EPLeaf(const EPLeaf &other) : node(other.node), next(other.next) {}
};

class EP {
private:
  ep_id_t id;
  std::shared_ptr<const BDD> bdd;

  EPNode *root;

  // Sorted by target priority and hit rates, from highest to lowest priority.
  // Leaves targeting the switch are prioritized over other targets.
  // Leaves with higher hit rates are prioritized over lower hit rates.
  // Don't forget to call the sorting method after adding new leaves.
  std::vector<EPLeaf> active_leaves;

  const TargetsView targets;
  const std::set<ep_id_t> ancestors;

  std::unordered_map<TargetType, nodes_t> targets_roots;

  Context ctx;
  EPMeta meta;

  mutable std::optional<pps_t> cached_tput_estimation;
  mutable std::optional<pps_t> cached_tput_speculation;

public:
  EP(std::shared_ptr<const BDD> bdd, const TargetsView &targets,
     const toml::table &config, const Profiler &profiler);

  EP(const EP &other, bool is_ancestor = true);

  ~EP();

  EP(EP &&other) = delete;
  EP &operator=(const EP *other) = delete;

  void process_leaf(EPNode *new_node, const std::vector<EPLeaf> &new_leaves,
                    bool process_node = true);
  void process_leaf(const Node *next_node);

  void replace_bdd(std::unique_ptr<BDD> new_bdd,
                   const translator_t &next_nodes_translator = translator_t(),
                   const translator_t &processed_nodes_translator = translator_t());

  ep_id_t get_id() const;
  const BDD *get_bdd() const;
  const EPNode *get_root() const;
  const std::vector<EPLeaf> &get_active_leaves() const;
  const TargetsView &get_targets() const;
  const nodes_t &get_target_roots(TargetType target) const;
  const std::set<ep_id_t> &get_ancestors() const;
  const Context &get_ctx() const;
  const EPMeta &get_meta() const;

  EPNode *get_mutable_root();
  Context &get_mutable_ctx();
  EPNode *get_mutable_node_by_id(ep_node_id_t id);

  std::vector<const EPNode *> get_prev_nodes() const;
  std::vector<const EPNode *> get_prev_nodes_of_current_target() const;
  std::vector<const EPNode *>
  get_nodes_by_type(const std::unordered_set<ModuleType> &types) const;

  bool has_target(TargetType type) const;
  const Node *get_next_node() const;

  EPLeaf pop_active_leaf();
  EPLeaf get_active_leaf() const;
  bool has_active_leaf() const;

  TargetType get_active_target() const;
  EPNode *get_node_by_id(ep_node_id_t id) const;

  // TODO: Improve the performance of this method.
  // Currently it goes through all the leaf's parents, extracts the generated
  // conditions, and traverses the hit rate tree (using the solver).
  hit_rate_t get_active_leaf_hit_rate() const;

  pps_t estimate_tput_pps() const;
  pps_t speculate_tput_pps() const;

  void debug() const;
  void debug_placements() const;
  void debug_hit_rate() const;
  void debug_active_leaves() const;

  void assert_integrity() const;

private:
  void sort_leaves();

  spec_impl_t peek_speculation_for_future_nodes(const spec_impl_t &base_speculation,
                                                const Node *anchor, nodes_t future_nodes,
                                                TargetType current_target,
                                                pps_t ingress) const;
  spec_impl_t get_best_speculation(const Node *node, TargetType current_target,
                                   const Context &ctx, const nodes_t &skip,
                                   pps_t ingress) const;
  bool is_better_speculation(const spec_impl_t &old_speculation,
                             const spec_impl_t &new_speculation, const Node *node,
                             TargetType current_target, pps_t ingress) const;
};