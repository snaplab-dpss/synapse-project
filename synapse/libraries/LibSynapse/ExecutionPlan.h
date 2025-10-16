#pragma once

#include <LibClone/PhysicalNetwork.h>
#include <LibSynapse/EPNode.h>
#include <LibSynapse/Meta.h>
#include <LibSynapse/Context.h>
#include <LibSynapse/Visualizers/EPVisualizer.h>
#include <LibBDD/BDD.h>
#include <LibBDD/Reorder.h>
#include <LibCore/Types.h>

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <list>

namespace LibSynapse {

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;
using LibBDD::BDDNode;
using LibBDD::Call;
using LibBDD::map_coalescing_objs_t;
using LibBDD::symbol_translation_t;

using LibClone::ComponentId;
using LibClone::InfrastructureNodeId;
using LibClone::PhysicalNetwork;

using translator_t = std::unordered_map<bdd_node_id_t, bdd_node_id_t>;
using ep_id_t      = u64;

class Profiler;
struct spec_impl_t;

struct EPLeaf {
  EPNode *node;
  const BDDNode *next;

  EPLeaf(EPNode *_node, const BDDNode *_next) : node(_node), next(_next) {}
  EPLeaf(const EPLeaf &other) : node(other.node), next(other.next) {}
};

struct complete_speculation_t {
  std::vector<spec_impl_t> speculations_per_node;
  Context final_ctx;
};

class EP {
private:
  ep_id_t id;
  std::shared_ptr<const BDD> bdd;
  std::unique_ptr<EPNode> root;

  // Sorted by target priority and hit rates, from highest to lowest priority.
  // Leaves targeting the switch are prioritized over other targets.
  // Leaves with higher hit rates are prioritized over lower hit rates.
  // Don't forget to call the sorting method after adding new leaves.
  std::list<EPLeaf> active_leaves;

  const TargetsView targets;
  const std::set<ep_id_t> ancestors;

  std::unordered_map<TargetType, bdd_node_ids_t> targets_roots;

  Context ctx;
  EPMeta meta;

  mutable std::optional<pps_t> cached_tput_estimation;
  mutable std::optional<pps_t> cached_tput_speculation;
  mutable std::optional<complete_speculation_t> cached_speculations;

public:
  EP(const BDD &bdd, const TargetsView &targets, const targets_config_t &targets_config, const Profiler &profiler);
  EP(const EP &other, bool is_ancestor = true);
  EP(EP &&other)                 = delete;
  EP &operator=(const EP *other) = delete;

  void process_leaf(EPNode *new_node, const std::vector<EPLeaf> &new_leaves, bool process_node = true);
  void process_leaf(const BDDNode *next_node);

  struct translation_data_t {
    const BDDNode *reordered_node;
    translator_t next_nodes_translator;
    translator_t processed_nodes_translator;
    std::vector<symbol_translation_t> translated_symbols;
  };

  void replace_bdd(std::unique_ptr<BDD> new_bdd);
  void replace_bdd(std::unique_ptr<BDD> new_bdd, const translation_data_t &translation_data);

  ep_id_t get_id() const { return id; }
  const BDD *get_bdd() const { return bdd.get(); }
  const EPNode *get_root() const { return root.get(); }
  const std::list<EPLeaf> &get_active_leaves() const { return active_leaves; }
  const TargetsView &get_targets() const { return targets; }
  const bdd_node_ids_t &get_target_roots(TargetType target) const { return targets_roots.at(target); }
  const std::set<ep_id_t> &get_ancestors() const { return ancestors; }
  const Context &get_ctx() const { return ctx; }
  const EPMeta &get_meta() const { return meta; }

  EPNode *get_mutable_root() { return root.get(); }
  Context &get_mutable_ctx() { return ctx; }
  EPLeaf pop_active_leaf();

  std::vector<const EPNode *> get_prev_nodes() const;
  std::vector<const EPNode *> get_prev_nodes_of_current_target() const;
  std::vector<const EPNode *> get_nodes_by_type(const std::unordered_set<ModuleType> &types) const;

  bool has_target(TargetType type) const;
  const BDDNode *get_next_node() const;
  const EPNode *get_leaf_ep_node_from_bdd_node(const BDDNode *node) const;
  EPLeaf get_active_leaf() const { return active_leaves.front(); }
  bool has_active_leaf() const { return !active_leaves.empty(); }
  TargetType get_active_target() const;
  hit_rate_t get_active_leaf_hit_rate() const;
  port_ingress_t get_node_egress(hit_rate_t hr, const EPNode *node) const;
  pps_t estimate_tput_pps() const;

  const TargetType get_target_by_id(const InstanceId id) const;

  // Sources of error:
  // 1. Speculative performance is calculated as we make the speculative decisions, so local speculative decisions don't take into
  // consideration future speculative decisions.
  //   - This makes the speculative performance optimistic.
  //   - Fixing this would require a recalculation of the speculative performance after all decisions were made.
  // 2. Speculative decisions that would perform BDD manipulations don't actually make them. Newer parts of the BDD are
  // abandoned during speculation, along with their hit rates.
  //   - This makes the speculation pessismistic, as part of the traffic will be lost.
  complete_speculation_t speculate() const;
  pps_t speculate_tput_pps() const;

  void debug() const;
  void debug_placements() const;
  void debug_hit_rate() const;
  void debug_active_leaves() const;
  void debug_speculations() const;
  void assert_integrity() const;

  void clear_caches() const;

private:
  void sort_leaves();

  struct speculation_target_t {
    const BDDNode *node;
    TargetType target;
  };

  std::list<speculation_target_t> get_nodes_targeted_for_speculation() const;

  enum class SpeculationStrategy {
    OneShot,
    WithLookahead,
    WithoutLookahead,
  };

  complete_speculation_t speculate(const Context &ctx, std::list<speculation_target_t> speculation_target_nodes, pps_t ingress,
                                   SpeculationStrategy lookahead = SpeculationStrategy::WithLookahead) const;
  spec_impl_t get_best_speculation(const speculation_target_t &speculation_target, const Context &ctx, pps_t ingress,
                                   const std::list<speculation_target_t> &speculation_target_nodes, SpeculationStrategy lookahead) const;
  bool is_better_speculation(const spec_impl_t &old_speculation, const spec_impl_t &new_speculation, const speculation_target_t &speculation_target,
                             pps_t ingress, const std::list<speculation_target_t> &speculation_target_nodes, SpeculationStrategy lookahead) const;

  struct tput_cmp_t {
    pps_t old_pps;
    pps_t new_pps;
  };

  // Compare the performance between an old speculation decision and a new one by checking their performance as if they were subjected to the nodes
  // ignored by the other one, and vise versa.
  tput_cmp_t compare_speculations_by_ignored_nodes(const spec_impl_t &old_speculation, const spec_impl_t &new_speculation,
                                                   const speculation_target_t &speculation_target, pps_t ingress,
                                                   const std::list<speculation_target_t> &speculation_target_nodes) const;

  // Compare the performance of an old speculation with a new one by checking their performance when considering all reachable BDD nodes from the
  // current node (down to the leaves).
  tput_cmp_t compare_speculations_with_reachable_nodes_lookahead(const spec_impl_t &old_speculation, const spec_impl_t &new_speculation,
                                                                 const speculation_target_t &speculation_target, pps_t ingress,
                                                                 const std::list<speculation_target_t> &speculation_target_nodes) const;

  // Compare the performance of an old speculation with a new one by checking their performance when considering _all_ unexplored BDD nodes.
  // This is much more expensive than compare_speculations_by_ignored_nodes and compare_speculations_with_reachable_nodes_lookahead, but it is useful
  // to break ties between them.
  tput_cmp_t compare_speculations_with_unexplored_nodes_lookahead(const spec_impl_t &old_speculation, const spec_impl_t &new_speculation,
                                                                  const speculation_target_t &speculation_target, pps_t ingress,
                                                                  const std::list<speculation_target_t> &speculation_target_nodes) const;
};

} // namespace LibSynapse
