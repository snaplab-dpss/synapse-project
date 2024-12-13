#include "execution_plan.h"
#include "node.h"
#include "visitor.h"
#include "../targets/targets.h"
#include "../exprs/solver.h"
#include "../profiler.h"
#include "../log.h"

static ep_id_t counter = 0;

static std::unordered_set<TargetType>
get_target_types(const targets_t &targets) {
  ASSERT(targets.size(), "No targets to get types from.");
  std::unordered_set<TargetType> targets_types;

  for (const Target *target : targets) {
    targets_types.insert(target->type);
  }

  return targets_types;
}

static TargetType get_initial_target(const targets_t &targets) {
  ASSERT(targets.size(), "No targets to get the initial target from.");
  return targets[0]->type;
}

static BDD *setup_bdd(const BDD *bdd) {
  BDD *new_bdd = new BDD(*bdd);

  delete_all_vector_key_operations_from_bdd(new_bdd);

  // Just to double check that we didn't break anything...
  new_bdd->assert_integrity();

  return new_bdd;
}

EP::EP(std::shared_ptr<const BDD> _bdd, const targets_t &_targets,
       const toml::table &_config, const Profiler &_profiler)
    : id(counter++), bdd(setup_bdd(_bdd.get())), root(nullptr),
      initial_target(get_initial_target(_targets)), targets(_targets),
      ctx(bdd.get(), targets, initial_target, _config, _profiler),
      meta(bdd.get(), get_target_types(targets), initial_target) {
  targets_roots[initial_target] = nodes_t({bdd->get_root()->get_id()});

  for (const Target *target : _targets) {
    if (target->type != initial_target) {
      targets_roots[target->type] = nodes_t();
    }
  }

  active_leaves.emplace_back(nullptr, bdd->get_root());
}

static std::set<ep_id_t> update_ancestors(const EP &other, bool is_ancestor) {
  std::set<ep_id_t> ancestors = other.get_ancestors();

  if (is_ancestor) {
    ancestors.insert(other.get_id());
  }

  return ancestors;
}

EP::EP(const EP &other, bool is_ancestor)
    : id(counter++), bdd(other.bdd),
      root(other.root ? other.root->clone(true) : nullptr),
      initial_target(other.initial_target), targets(other.targets),
      ancestors(update_ancestors(other, is_ancestor)),
      targets_roots(other.targets_roots), ctx(other.ctx), meta(other.meta) {
  if (!root) {
    ASSERT(other.active_leaves.size() == 1, "No root and multiple leaves.");
    active_leaves.emplace_back(nullptr, bdd->get_root());
    return;
  }

  for (const EPLeaf &leaf : other.active_leaves) {
    EPNode *leaf_node = root->get_mutable_node_by_id(leaf.node->get_id());
    ASSERT(leaf_node, "Leaf node not found in the cloned tree.");
    active_leaves.emplace_back(leaf_node, leaf.next);
  }

  sort_leaves();
}

EP::~EP() {
  if (root) {
    delete root;
    root = nullptr;
  }
}

const EPMeta &EP::get_meta() const { return meta; }

ep_id_t EP::get_id() const { return id; }

const EPNode *EP::get_root() const { return root; }

EPNode *EP::get_mutable_root() { return root; }

const std::vector<EPLeaf> &EP::get_active_leaves() const {
  return active_leaves;
}

const targets_t &EP::get_targets() const { return targets; }

const nodes_t &EP::get_target_roots(TargetType target) const {
  ASSERT(targets_roots.find(target) != targets_roots.end(),
         "Target not found in the roots map.");
  return targets_roots.at(target);
}

const std::set<ep_id_t> &EP::get_ancestors() const { return ancestors; }

const BDD *EP::get_bdd() const { return bdd.get(); }

std::vector<const EPNode *> EP::get_prev_nodes() const {
  std::vector<const EPNode *> prev_nodes;

  EPLeaf current = get_active_leaf();
  const EPNode *node = current.node;

  while (node) {
    prev_nodes.push_back(node);
    node = node->get_prev();
  }

  return prev_nodes;
}

std::vector<const EPNode *> EP::get_prev_nodes_of_current_target() const {
  std::vector<const EPNode *> prev_nodes;

  TargetType target = get_active_target();
  EPLeaf current = get_active_leaf();
  const EPNode *node = current.node;

  while (node) {
    const Module *module = node->get_module();

    if (module->get_target() != target) {
      break;
    }

    prev_nodes.push_back(node);
    node = node->get_prev();
  }

  return prev_nodes;
}

std::vector<const EPNode *>
EP::get_nodes_by_type(const std::unordered_set<ModuleType> &types) const {
  std::vector<const EPNode *> found;

  if (!root) {
    return found;
  }

  root->visit_nodes([&found, &types](const EPNode *node) {
    const Module *module = node->get_module();
    ASSERT(module, "Node without a module");

    if (types.find(module->get_type()) != types.end()) {
      found.push_back(node);
    }

    return EPNodeVisitAction::Continue;
  });

  return found;
}

bool EP::has_target(TargetType type) const {
  auto found_it = std::find_if(
      targets.begin(), targets.end(),
      [type](const Target *target) { return target->type == type; });

  return found_it != targets.end();
}

const Context &EP::get_ctx() const { return ctx; }

Context &EP::get_mutable_ctx() { return ctx; }

const Node *EP::get_next_node() const {
  if (!has_active_leaf()) {
    return nullptr;
  }

  EPLeaf active_leaf = get_active_leaf();
  return active_leaf.next;
}

EPLeaf EP::pop_active_leaf() {
  ASSERT(!active_leaves.empty(), "No active leaf");
  EPLeaf leaf = active_leaves.front();
  active_leaves.erase(active_leaves.begin());
  return leaf;
}

EPLeaf EP::get_active_leaf() const { return active_leaves.front(); }

bool EP::has_active_leaf() const { return !active_leaves.empty(); }

TargetType EP::get_active_target() const {
  if (!root) {
    return initial_target;
  }

  ASSERT(has_active_leaf(), "No active leaf");
  EPLeaf active_leaf = get_active_leaf();

  return active_leaf.node->get_module()->get_next_target();
}

void EP::process_leaf(const Node *next_node) {
  TargetType current_target = get_active_target();
  EPLeaf active_leaf = pop_active_leaf();

  meta.process_node(active_leaf.next, current_target);

  if (next_node) {
    active_leaves.emplace_back(active_leaf.node, next_node);
    sort_leaves();
  }
}

void EP::process_leaf(EPNode *new_node, const std::vector<EPLeaf> &new_leaves,
                      bool process_node) {
  TargetType current_target = get_active_target();
  EPLeaf active_leaf = pop_active_leaf();

  if (!root) {
    root = new_node;
  } else {
    active_leaf.node->set_children(new_node);
    new_node->set_prev(active_leaf.node);
  }

  meta.update(active_leaf, new_node, process_node);

  meta.depth++;

  for (const EPLeaf &new_leaf : new_leaves) {
    if (!new_leaf.next) {
      continue;
    }

    if (new_leaf.node != new_node) {
      meta.update(active_leaf, new_leaf.node, process_node);
    }

    const Module *module = new_leaf.node->get_module();
    TargetType next_target = module->get_next_target();
    node_id_t next_node_id = new_leaf.next->get_id();

    if (next_target != current_target) {
      targets_roots[next_target].insert(next_node_id);
    }
  }

  for (const EPLeaf &new_leaf : new_leaves) {
    if (!new_leaf.next)
      continue;

    const Module *module = new_leaf.node->get_module();
    TargetType next_target = module->get_next_target();

    active_leaves.push_back(new_leaf);
  }

  sort_leaves();
}

void EP::replace_bdd(const BDD *new_bdd,
                     const translator_t &next_nodes_translator,
                     const translator_t &processed_nodes_translator) {
  auto translate_next_node = [&next_nodes_translator](node_id_t id) {
    auto found_it = next_nodes_translator.find(id);
    return (found_it != next_nodes_translator.end()) ? found_it->second : id;
  };

  auto translate_processed_node = [&processed_nodes_translator](node_id_t id) {
    auto found_it = processed_nodes_translator.find(id);
    return (found_it != processed_nodes_translator.end()) ? found_it->second
                                                          : id;
  };

  for (EPLeaf &leaf : active_leaves) {
    ASSERT(leaf.next, "Active leaf without a next node");

    node_id_t new_id = translate_next_node(leaf.next->get_id());
    const Node *new_node = new_bdd->get_node_by_id(new_id);
    ASSERT(new_node, "New node not found in the new BDD.");

    leaf.next = new_node;
  }

  if (!root) {
    return;
  }

  root->visit_mutable_nodes([&new_bdd, translate_processed_node](EPNode *node) {
    Module *module = node->get_mutable_module();

    const Node *node_bdd = module->get_node();
    node_id_t target_id = translate_processed_node(node_bdd->get_id());

    const Node *new_node = new_bdd->get_node_by_id(target_id);
    ASSERT(new_node, "Node (id=%lu) not found in the new BDD.", target_id);

    module->set_node(new_node);

    return EPNodeVisitAction::Continue;
  });

  meta.update_total_bdd_nodes(new_bdd);

  // Replacing the BDD might change the hit rate estimations.
  ctx.get_profiler().clear_cache();

  // Reset the BDD only here, because we might lose the final reference to it
  // and we needed the old nodes to find the new ones.
  bdd.reset(new_bdd);

  sort_leaves();
}

void EP::debug() const {
  Log::dbg() << "ID: " << id << "\n";
  Log::dbg() << "Ancestors:";
  for (ep_id_t ancestor : ancestors) {
    Log::dbg() << "  " << ancestor;
  }
  Log::dbg() << "\n";
  debug_hit_rate();
  debug_placements();
  debug_active_leaves();
  ctx.debug();
}

void EP::debug_placements() const {
  Log::dbg() << "Implementations:\n";
  const std::unordered_map<addr_t, DSImpl> &impls = ctx.get_ds_impls();
  for (const auto &[obj, impl] : impls) {
    Log::dbg() << "  " << obj << " -> " << impl << "\n";
  }
}

void EP::debug_hit_rate() const {
  const Profiler &profiler = ctx.get_profiler();
  profiler.debug();
}

void EP::debug_active_leaves() const {
  const Profiler &profiler = ctx.get_profiler();
  Log::dbg() << "Active leaves:\n";
  for (const EPLeaf &leaf : active_leaves) {
    ASSERT(leaf.next, "Active leaf without a next node");
    Log::dbg() << "  " << leaf.next->dump(true, true);
    if (leaf.node) {
      Log::dbg() << " | " << leaf.node->dump();
      Log::dbg() << " | HR=" << profiler.get_hr(leaf.node) << "\n";
    }
  }
}

void EP::assert_integrity() const {
  if (!Log::is_debug_active()) {
    return;
  }

  std::vector<const EPNode *> nodes{root};

  while (nodes.size()) {
    const EPNode *node = nodes.back();
    nodes.pop_back();

    ASSERT(node, "Null node");
    ASSERT(node->get_module(), "Node without a module");

    const Module *module = node->get_module();
    const Node *bdd_node = module->get_node();
    ASSERT(bdd_node, "Module without a node");

    const Node *found_bdd_node = bdd->get_node_by_id(bdd_node->get_id());
    ASSERT(bdd_node == found_bdd_node, "Node not found in the BDD");

    for (const EPNode *child : node->get_children()) {
      ASSERT(child, "Null child");
      ASSERT(child->get_prev() == node, "Child without the correct parent");
      nodes.push_back(child);
    }
  }

  for (const auto &[target, roots] : targets_roots) {
    for (const node_id_t root_id : roots) {
      const Node *bdd_node = bdd->get_node_by_id(root_id);
      ASSERT(bdd_node, "Root node not found in the BDD");

      const Node *found_bdd_node = bdd->get_node_by_id(bdd_node->get_id());
      ASSERT(bdd_node == found_bdd_node, "Root node not found in the BDD");
    }
  }

  for (const EPLeaf &leaf : active_leaves) {
    const Node *next = leaf.next;
    ASSERT(next, "Active leaf without a next node");

    const Node *found_next = bdd->get_node_by_id(next->get_id());
    ASSERT(next == found_next, "Next node not found in the BDD");
  }

  bdd->assert_integrity();
}

hit_rate_t EP::get_active_leaf_hit_rate() const {
  EPLeaf active_leaf = get_active_leaf();

  if (!active_leaf.node) {
    return 1;
  }

  return ctx.get_profiler().get_hr(active_leaf.node);
}

void EP::sort_leaves() {
  auto prioritize_switch_and_hot_paths = [this](const EPLeaf &l1,
                                                const EPLeaf &l2) {
    // Only the first leaf may have no EPNode.
    ASSERT(l1.node, "Leaf without a node");
    ASSERT(l2.node, "Leaf without a node");

    if (l1.node->get_module()->get_next_target() != initial_target &&
        l2.node->get_module()->get_next_target() == initial_target) {
      return false;
    }

    if (l1.node->get_module()->get_next_target() == initial_target &&
        l2.node->get_module()->get_next_target() != initial_target) {
      return true;
    }

    hit_rate_t l1_hr = ctx.get_profiler().get_hr(l1.node);
    hit_rate_t l2_hr = ctx.get_profiler().get_hr(l2.node);

    return l1_hr > l2_hr;
  };

  std::sort(active_leaves.begin(), active_leaves.end(),
            prioritize_switch_and_hot_paths);
}

std::string spec2str(const spec_impl_t &speculation, const BDD *bdd) {
  std::stringstream ss;

  ss << speculation.decision.module;
  ss << " ";
  if (!speculation.skip.empty()) {
    ss << "skip={";
    for (node_id_t skip : speculation.skip)
      ss << skip << ",";
    ss << "} ";
  }
  ss << "(" << bdd->get_node_by_id(speculation.decision.node)->dump(true, true)
     << ")";

  return ss.str();
}

std::string speculations2str(const EP *ep,
                             const std::vector<spec_impl_t> &speculations) {
  std::stringstream ss;

  ss << "Speculating EP " << ep->get_id() << ":\n";

  for (const spec_impl_t &speculation : speculations) {
    const Node *node = ep->get_bdd()->get_node_by_id(speculation.decision.node);

    ss << "  ";
    ss << spec2str(speculation, ep->get_bdd());
    ss << "\n";
  }

  ss << "\n";

  return ss.str();
}

spec_impl_t EP::peek_speculation_for_future_nodes(
    const spec_impl_t &base_speculation, const Node *anchor,
    nodes_t future_nodes, TargetType current_target, pps_t ingress) const {
  future_nodes.erase(anchor->get_id());

  spec_impl_t speculation = base_speculation;
  std::vector<spec_impl_t> speculations;

  anchor->visit_nodes([this, &speculation, &speculations, current_target,
                       ingress, &future_nodes](const Node *node) {
    if (future_nodes.empty()) {
      return NodeVisitAction::Stop;
    }

    if (future_nodes.find(node->get_id()) == future_nodes.end()) {
      return NodeVisitAction::Continue;
    }

    future_nodes.erase(node->get_id());

    speculation = get_best_speculation(node, current_target, speculation.ctx,
                                       speculation.skip, ingress);
    speculations.push_back(speculation);

    if (speculation.next_target.has_value() &&
        speculation.next_target != current_target) {
      return NodeVisitAction::SkipChildren;
    }

    return NodeVisitAction::Continue;
  });

  return speculation;
}

static nodes_t filter_away_nodes(const nodes_t &nodes, const nodes_t &filter) {
  nodes_t result;

  for (node_id_t node_id : nodes) {
    if (filter.find(node_id) == filter.end()) {
      result.insert(node_id);
    }
  }

  return result;
}

// Compare the performance of an old speculation if it were subjected to the
// nodes ignored by the new speculation, and vise versa.
bool EP::is_better_speculation(const spec_impl_t &old_speculation,
                               const spec_impl_t &new_speculation,
                               const Node *node, TargetType current_target,
                               pps_t ingress) const {
  nodes_t old_future_nodes =
      filter_away_nodes(new_speculation.skip, old_speculation.skip);
  nodes_t new_future_nodes =
      filter_away_nodes(old_speculation.skip, new_speculation.skip);

  spec_impl_t peek_old = peek_speculation_for_future_nodes(
      old_speculation, node, old_future_nodes, current_target, ingress);

  spec_impl_t peek_new = peek_speculation_for_future_nodes(
      new_speculation, node, new_future_nodes, current_target, ingress);

  pps_t old_pps = peek_old.ctx.get_perf_oracle().estimate_tput(ingress);
  pps_t new_pps = peek_new.ctx.get_perf_oracle().estimate_tput(ingress);

  if (old_pps != new_pps) {
    return new_pps > old_pps;
  }

  // Speeding things up, we don't need to check for small differences in
  // throughput.
  if (old_pps <= STABLE_TPUT_PRECISION) {
    return false;
  }

  old_future_nodes.clear();
  new_future_nodes.clear();

  for (const Node *child : node->get_children(true)) {
    if (old_speculation.skip.find(child->get_id()) ==
        old_speculation.skip.end()) {
      old_future_nodes.insert(child->get_id());
    }

    if (new_speculation.skip.find(child->get_id()) ==
        new_speculation.skip.end()) {
      new_future_nodes.insert(child->get_id());
    }
  }

  peek_old = peek_speculation_for_future_nodes(
      old_speculation, node, old_future_nodes, current_target, ingress);

  peek_new = peek_speculation_for_future_nodes(
      new_speculation, node, new_future_nodes, current_target, ingress);

  old_pps = peek_old.ctx.get_perf_oracle().estimate_tput(ingress);
  new_pps = peek_new.ctx.get_perf_oracle().estimate_tput(ingress);

  return new_pps > old_pps;
}

spec_impl_t EP::get_best_speculation(const Node *node,
                                     TargetType current_target,
                                     const Context &ctx, const nodes_t &skip,
                                     pps_t ingress) const {
  std::optional<spec_impl_t> best;

  const targets_t &targets = get_targets();

  for (const Target *target : targets) {
    if (target->type != current_target) {
      continue;
    }

    for (const ModuleGenerator *modgen : target->module_generators) {
      std::optional<spec_impl_t> spec = modgen->speculate(this, node, ctx);

      if (!spec.has_value()) {
        continue;
      }

      spec->skip.insert(skip.begin(), skip.end());

      if (!best.has_value()) {
        best = *spec;
        continue;
      }

      ASSERT(best.has_value(), "No best speculation");
      ASSERT(spec.has_value(), "No speculation");

      bool is_better =
          is_better_speculation(*best, *spec, node, current_target, ingress);

      if (is_better) {
        best = *spec;
      }
    }
  }

  if (!best.has_value()) {
    ctx.debug();
    // EPViz::visualize(this, false);
    // BDDViz::visualize(bdd.get(), false);

    PANIC("No module to speculative execute\n"
          "EP:     %lu\n"
          "Target: %s\n"
          "Node:   %s\n",
          id, to_string(current_target).c_str(), node->dump(true).c_str());
  }

  return *best;
}

static pps_t find_stable_tput(pps_t ingress,
                              std::function<pps_t(pps_t)> estimator) {
  pps_t egress = 0;

  pps_t smallest_unstable = ingress;
  pps_t prev_ingress = ingress;
  pps_t diff = smallest_unstable;

  // Algorithm for converging to a stable throughput (basically a binary
  // search). This hopefully doesn't take many iterations...
  while (diff > STABLE_TPUT_PRECISION) {
    prev_ingress = ingress;
    egress = estimator(ingress);

    if (egress < ingress) {
      smallest_unstable = ingress;
      ingress = (ingress + egress) / 2;
    } else {
      ingress = (ingress + smallest_unstable) / 2;
    }

    diff = ingress > prev_ingress ? ingress - prev_ingress
                                  : prev_ingress - ingress;
  }

  return egress;
}

// Sources of error:
// 1. Speculative performance is calculated as we make the speculative
// decisions, so local speculative decisions don't take into consideration
// future speculative decisions.
//   - This makes the speculative performance optimistic.
//   - Fixing this would require a recalculation of the speculative performance
//   after all decisions were made.
// 2. Speculative decisions that would perform BDD manipulations don't actually
// make them. Newer parts of the BDD are abandoned during speculation, along
// with their hit rates.
//   - This makes the speculation pessismistic, as part of the traffic will be
//  lost.
pps_t EP::speculate_tput_pps() const {
  if (cached_tput_speculation.has_value()) {
    return *cached_tput_speculation;
  }

  pps_t ingress = estimate_tput_pps();

  std::vector<spec_impl_t> speculations;
  Context spec_ctx = ctx;
  nodes_t skip;

  Context other = std::move(Context(spec_ctx));

  struct spec_cookie_t : public cookie_t {
    TargetType target;

    spec_cookie_t(TargetType _target) : target(_target) {}

    virtual spec_cookie_t *clone() const override {
      return new spec_cookie_t(*this);
    }
  };

  for (const EPLeaf &leaf : active_leaves) {
    if (leaf.node &&
        leaf.node->get_module()->get_next_target() != initial_target) {
      continue;
    }

    ASSERT(leaf.next, "Active leaf without a next node");
    leaf.next->visit_nodes([this, &speculations, &spec_ctx, &skip,
                            ingress](const Node *node) {
      if (skip.find(node->get_id()) != skip.end()) {
        return NodeVisitAction::Continue;
      }

      if (ctx.get_profiler().get_hr(node) == 0) {
        skip.insert(node->get_id());
        return NodeVisitAction::Continue;
      }

      spec_impl_t speculation =
          get_best_speculation(node, initial_target, spec_ctx, skip, ingress);
      speculations.push_back(speculation);

      spec_ctx = speculation.ctx;
      skip = speculation.skip;

      if (speculation.next_target.has_value()) {
        // Just ignore if we change the target, we only care about the
        // switch nodes for now.
        return NodeVisitAction::SkipChildren;
      }

      return NodeVisitAction::Continue;
    });
  }

  auto egress_from_ingress = [spec_ctx](pps_t ingress) {
    return spec_ctx.get_perf_oracle().estimate_tput(ingress);
  };

  pps_t egress = find_stable_tput(ingress, egress_from_ingress);
  cached_tput_speculation = egress;

  // if (id == 0) {
  //   Log::dbg() << speculations2str(this, speculations);
  //   spec_ctx.debug();
  //   std::cerr << "Ingress: "
  //             << tput2str(
  //                    pps2bps(ingress,
  //                    ctx.get_profiler().get_avg_pkt_bytes()), "bps", true)
  //             << "\n";
  //   std::cerr << "Egress from ingress: "
  //             << tput2str(pps2bps(egress_from_ingress(ingress),
  //                                 ctx.get_profiler().get_avg_pkt_bytes()),
  //                         "bps", true)
  //             << "\n";
  //   std::cerr << "Stable egress: "
  //             << tput2str(
  //                    pps2bps(egress, ctx.get_profiler().get_avg_pkt_bytes()),
  //                    "bps", true)
  //             << "\n";
  //   // BDDViz::visualize(bdd.get(), false);
  //   // EPViz::visualize(this, false);
  //   // ProfilerViz::visualize(bdd.get(), spec_ctx.get_profiler(), false);
  //   // debug_active_leaves();
  //   DEBUG_PAUSE
  // }

  return egress;
}

pps_t EP::estimate_tput_pps() const {
  if (cached_tput_estimation.has_value()) {
    return *cached_tput_estimation;
  }

  pps_t max_ingress = ctx.get_perf_oracle().get_max_input_pps();

  auto egress_from_ingress = [this](pps_t ingress) {
    return ctx.get_perf_oracle().estimate_tput(ingress);
  };

  pps_t egress = find_stable_tput(max_ingress, egress_from_ingress);
  cached_tput_estimation = egress;

  return egress;
}