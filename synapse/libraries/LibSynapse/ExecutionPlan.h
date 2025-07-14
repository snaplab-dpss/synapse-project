#pragma once

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

namespace LibSynapse {

using LibBDD::BDD;
using LibBDD::bdd_node_id_t;
using LibBDD::bdd_node_ids_t;
using LibBDD::BDDNode;
using LibBDD::Call;
using LibBDD::map_coalescing_objs_t;
using LibBDD::translated_symbol_t;

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

  std::unordered_map<TargetType, bdd_node_ids_t> targets_roots;

  Context ctx;
  EPMeta meta;

  mutable std::optional<pps_t> cached_tput_estimation;
  mutable std::optional<pps_t> cached_tput_speculation;
  mutable std::optional<std::vector<spec_impl_t>> cached_speculations;

public:
  EP(const BDD &bdd, const TargetsView &targets, const targets_config_t &targets_config, const Profiler &profiler);
  EP(const EP &other, bool is_ancestor = true);
  EP(EP &&other)                 = delete;
  EP &operator=(const EP *other) = delete;

  ~EP();

  void process_leaf(EPNode *new_node, const std::vector<EPLeaf> &new_leaves, bool process_node = true);
  void process_leaf(const BDDNode *next_node);

  struct translation_data_t {
    const BDDNode *reordered_node;
    translator_t next_nodes_translator;
    translator_t processed_nodes_translator;
    std::vector<translated_symbol_t> translated_symbols;
  };

  void replace_bdd(std::unique_ptr<BDD> new_bdd);
  void replace_bdd(std::unique_ptr<BDD> new_bdd, const translation_data_t &translation_data);

  ep_id_t get_id() const;
  const BDD *get_bdd() const;
  const EPNode *get_root() const;
  const std::vector<EPLeaf> &get_active_leaves() const;
  const TargetsView &get_targets() const;
  const bdd_node_ids_t &get_target_roots(TargetType target) const;
  const std::set<ep_id_t> &get_ancestors() const;
  const Context &get_ctx() const;
  const EPMeta &get_meta() const;

  EPNode *get_mutable_root();
  Context &get_mutable_ctx();
  EPNode *get_mutable_node_by_id(ep_node_id_t id);
  EPLeaf pop_active_leaf();

  std::vector<const EPNode *> get_prev_nodes() const;
  std::vector<const EPNode *> get_prev_nodes_of_current_target() const;
  std::vector<const EPNode *> get_nodes_by_type(const std::unordered_set<ModuleType> &types) const;

  bool has_target(TargetType type) const;
  const BDDNode *get_next_node() const;
  EPLeaf get_active_leaf() const;
  bool has_active_leaf() const;
  TargetType get_active_target() const;
  EPNode *get_node_by_id(ep_node_id_t id) const;
  hit_rate_t get_active_leaf_hit_rate() const;
  port_ingress_t get_node_egress(hit_rate_t hr, const EPNode *node) const;
  port_ingress_t get_node_egress(hit_rate_t hr, std::vector<int> past_recirculations) const;
  pps_t estimate_tput_pps() const;
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
  spec_impl_t peek_speculation_for_future_nodes(const spec_impl_t &base_speculation, const BDDNode *anchor, bdd_node_ids_t future_nodes,
                                                TargetType current_target, pps_t ingress) const;
  spec_impl_t get_best_speculation(const BDDNode *node, TargetType current_target, const Context &ctx, const bdd_node_ids_t &skip,
                                   pps_t ingress) const;
  bool is_better_speculation(const spec_impl_t &old_speculation, const spec_impl_t &new_speculation, const BDDNode *node, TargetType current_target,
                             pps_t ingress) const;
};

} // namespace LibSynapse