#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Modules/Module.h>
#include <LibSynapse/Modules/ModuleFactory.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>
#include <LibCore/Debug.h>

namespace LibSynapse {

namespace {

using LibBDD::BDDNodeType;
using LibBDD::BDDNodeVisitAction;
using LibBDD::call_t;
using LibCore::expr_addr_to_obj_addr;
using LibCore::pps2bps;
using LibCore::tput2str;

constexpr const pps_t STABLE_TPUT_PRECISION        = 100;
constexpr const double STABLE_TPUT_ACCEPTABLE_LOSS = 0.01;

bdd_node_ids_t filter_away_nodes(const bdd_node_ids_t &nodes, const bdd_node_ids_t &filter) {
  bdd_node_ids_t result;

  for (bdd_node_id_t node_id : nodes) {
    if (filter.find(node_id) == filter.end()) {
      result.insert(node_id);
    }
  }

  return result;
}

struct tput_estimation_t {
  pps_t ingress;
  pps_t egress_estimation;
  pps_t unavoidable_drop;
};

pps_t find_stable_tput(pps_t ingress, std::function<tput_estimation_t(pps_t)> estimator) {
  pps_t egress = 0;

  pps_t prev_ingress = ingress;
  pps_t precision    = STABLE_TPUT_PRECISION + 1;
  pps_t delta        = ingress;
  pps_t floor        = 0;
  pps_t ceil         = ingress;

  // Algorithm for converging to a stable throughput (basically a binary search).
  // Hopefully this won't take many iterations...
  while (precision > STABLE_TPUT_PRECISION) {
    tput_estimation_t estimation = estimator(ingress);

    prev_ingress = ingress;
    egress       = estimation.egress_estimation + estimation.unavoidable_drop;
    delta        = ingress - egress;

    assert(egress <= ingress);

    if (delta <= STABLE_TPUT_PRECISION) {
      floor = ingress;
    } else {
      ceil = ingress;
    }

    ingress   = (floor + ceil) / 2;
    precision = ingress > prev_ingress ? ingress - prev_ingress : prev_ingress - ingress;
  }

  return egress;
}

ep_id_t ep_id_counter = 0;

void delete_all_unused_vector_key_operations_from_bdd(BDD *bdd) {
  // 1. Get all map operations.
  // 2. Remove all vector operations storing the key.
  std::unordered_set<addr_t> maps;

  bdd->get_root()->visit_nodes([&maps](const BDDNode *node) {
    if (node->get_type() != BDDNodeType::Call) {
      return BDDNodeVisitAction::Continue;
    }

    const Call *call_node = dynamic_cast<const Call *>(node);
    const call_t &call    = call_node->get_call();

    if (call.function_name != "map_get" && call.function_name != "map_put" && call.function_name != "map_erase") {
      return BDDNodeVisitAction::Continue;
    }

    klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
    const addr_t obj               = expr_addr_to_obj_addr(obj_expr);

    maps.insert(obj);

    return BDDNodeVisitAction::Continue;
  });

  // There are more efficient ways of doing this (that don't involve traversing the entire BDD every single time), but this is a quick and
  // dirty way of doing it.
  for (addr_t map : maps) {
    bdd->delete_vector_key_operations(map);
  }
}

BDD *setup_bdd(const BDD &bdd) {
  BDD *new_bdd = new BDD(bdd);
  delete_all_unused_vector_key_operations_from_bdd(new_bdd);
  return new_bdd;
}

std::set<ep_id_t> update_ancestors(const EP &other, bool is_ancestor) {
  std::set<ep_id_t> ancestors = other.get_ancestors();

  if (is_ancestor) {
    ancestors.insert(other.get_id());
  }

  return ancestors;
}

std::string spec2str(const spec_impl_t &speculation, const BDD *bdd) {
  std::stringstream ss;

  ss << speculation.decision.module;
  ss << " ";
  if (!speculation.skip.empty()) {
    ss << "skip={";
    for (bdd_node_id_t skip : speculation.skip)
      ss << skip << ",";
    ss << "} ";
  }
  ss << "(" << bdd->get_node_by_id(speculation.decision.node)->dump(true, true) << ")";

  return ss.str();
}

std::string speculations2str(const EP *ep, const std::vector<spec_impl_t> &speculations) {
  std::stringstream ss;

  ss << "Speculating EP " << ep->get_id() << ":\n";
  for (const spec_impl_t &speculation : speculations) {
    ss << "  " << spec2str(speculation, ep->get_bdd()) << "\n";
  }
  ss << "\n";

  return ss.str();
}

} // namespace

EP::EP(const BDD &_bdd, const TargetsView &_targets, const targets_config_t &_targets_config, const Profiler &_profiler)
    : id(ep_id_counter++), bdd(setup_bdd(_bdd)), root(nullptr), targets(_targets), ctx(bdd.get(), _targets, _targets_config, _profiler),
      meta(bdd.get(), targets) {
  TargetType initial_target     = targets.get_initial_target().type;
  targets_roots[initial_target] = bdd_node_ids_t({bdd->get_root()->get_id()});

  for (const TargetView &target : targets.elements) {
    if (target.type != initial_target) {
      targets_roots[target.type] = bdd_node_ids_t();
    }
  }

  active_leaves.emplace_back(nullptr, bdd->get_root());
}

EP::EP(const EP &other, bool is_ancestor)
    : id(ep_id_counter++), bdd(other.bdd), root(other.root ? other.root->clone(true) : nullptr), targets(other.targets),
      ancestors(update_ancestors(other, is_ancestor)), targets_roots(other.targets_roots), ctx(other.ctx), meta(other.meta) {
  if (!root) {
    assert(other.active_leaves.size() == 1 && "No root and multiple leaves.");
    active_leaves.emplace_back(nullptr, bdd->get_root());
    return;
  }

  for (const EPLeaf &leaf : other.active_leaves) {
    EPNode *leaf_node = root->get_mutable_node_by_id(leaf.node->get_id());
    assert(leaf_node && "Leaf node not found in the cloned tree.");
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

const std::vector<EPLeaf> &EP::get_active_leaves() const { return active_leaves; }

const TargetsView &EP::get_targets() const { return targets; }

const bdd_node_ids_t &EP::get_target_roots(TargetType target) const {
  assert(targets_roots.find(target) != targets_roots.end() && "Target not found in the roots map.");
  return targets_roots.at(target);
}

const std::set<ep_id_t> &EP::get_ancestors() const { return ancestors; }

const BDD *EP::get_bdd() const { return bdd.get(); }

std::vector<const EPNode *> EP::get_prev_nodes() const {
  std::vector<const EPNode *> prev_nodes;

  EPLeaf current     = get_active_leaf();
  const EPNode *node = current.node;

  while (node) {
    prev_nodes.push_back(node);
    node = node->get_prev();
  }

  return prev_nodes;
}

std::vector<const EPNode *> EP::get_prev_nodes_of_current_target() const {
  std::vector<const EPNode *> prev_nodes;

  TargetType target  = get_active_target();
  EPLeaf current     = get_active_leaf();
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

std::vector<const EPNode *> EP::get_nodes_by_type(const std::unordered_set<ModuleType> &types) const {
  std::vector<const EPNode *> found;

  if (!root) {
    return found;
  }

  root->visit_nodes([&found, &types](const EPNode *node) {
    const Module *module = node->get_module();
    assert(module && "BDDNode without a module");

    if (types.find(module->get_type()) != types.end()) {
      found.push_back(node);
    }

    return EPNodeVisitAction::Continue;
  });

  return found;
}

bool EP::has_target(TargetType type) const {
  auto found_it = std::find_if(targets.elements.begin(), targets.elements.end(), [type](const TargetView &target) { return target.type == type; });

  return found_it != targets.elements.end();
}

const Context &EP::get_ctx() const { return ctx; }

Context &EP::get_mutable_ctx() { return ctx; }

const BDDNode *EP::get_next_node() const {
  if (!has_active_leaf()) {
    return nullptr;
  }

  EPLeaf active_leaf = get_active_leaf();
  return active_leaf.next;
}

EPLeaf EP::pop_active_leaf() {
  assert(!active_leaves.empty() && "No active leaf");
  EPLeaf leaf = active_leaves.front();
  active_leaves.erase(active_leaves.begin());
  return leaf;
}

EPLeaf EP::get_active_leaf() const { return active_leaves.front(); }

bool EP::has_active_leaf() const { return !active_leaves.empty(); }

TargetType EP::get_active_target() const {
  if (!root) {
    TargetType initial_target = targets.get_initial_target().type;
    return initial_target;
  }

  assert(has_active_leaf() && "No active leaf");
  EPLeaf active_leaf = get_active_leaf();

  return active_leaf.node->get_module()->get_next_target();
}

void EP::process_leaf(const BDDNode *next_node) {
  clear_caches();

  TargetType current_target = get_active_target();
  EPLeaf active_leaf        = pop_active_leaf();

  meta.process_node(active_leaf.next, current_target);

  if (next_node) {
    active_leaves.emplace_back(active_leaf.node, next_node);
    sort_leaves();
  }
}

void EP::process_leaf(EPNode *new_node, const std::vector<EPLeaf> &new_leaves, bool process_node) {
  clear_caches();

  const TargetType current_target = get_active_target();
  const EPLeaf active_leaf        = pop_active_leaf();

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

    const Module *module             = new_leaf.node->get_module();
    const TargetType next_target     = module->get_next_target();
    const bdd_node_id_t next_node_id = new_leaf.next->get_id();

    if (next_target != current_target) {
      targets_roots[next_target].insert(next_node_id);
    }
  }

  for (const EPLeaf &new_leaf : new_leaves) {
    if (new_leaf.next) {
      active_leaves.push_back(new_leaf);
    }
  }

  sort_leaves();
}

void EP::replace_bdd(std::unique_ptr<BDD> new_bdd) {
  clear_caches();

  for (EPLeaf &leaf : active_leaves) {
    assert(leaf.next && "Active leaf without a next node");

    const bdd_node_id_t leaf_id = leaf.next->get_id();
    const BDDNode *new_node     = new_bdd->get_node_by_id(leaf_id);
    assert(new_node && "New node not found in the new BDD.");

    leaf.next = new_node;
  }

  if (root) {
    root->visit_mutable_nodes([&new_bdd](EPNode *node) {
      Module *module = node->get_mutable_module();

      const BDDNode *node_bdd       = module->get_node();
      const bdd_node_id_t target_id = node_bdd->get_id();

      const BDDNode *new_node = new_bdd->get_node_by_id(target_id);
      assert(new_node && "BDDNode not found in the new BDD");

      module->set_node(new_node);

      return EPNodeVisitAction::Continue;
    });
  }

  meta.update_total_bdd_nodes(new_bdd.get());

  // Replacing the BDD might change the hit rate estimations.
  ctx.get_profiler().clear_cache();

  // Reset the BDD only here, because we might lose the final reference to it and we needed the old nodes to find the new ones.
  bdd = std::move(new_bdd);

  sort_leaves();
}

void EP::replace_bdd(std::unique_ptr<BDD> new_bdd, const translation_data_t &translation_data) {
  clear_caches();

  auto translate_next_node = [&translation_data](bdd_node_id_t node_id) {
    auto found_it = translation_data.next_nodes_translator.find(node_id);
    return (found_it != translation_data.next_nodes_translator.end()) ? found_it->second : node_id;
  };

  auto translate_processed_node = [&translation_data](bdd_node_id_t node_id) {
    auto found_it = translation_data.processed_nodes_translator.find(node_id);
    return (found_it != translation_data.processed_nodes_translator.end()) ? found_it->second : node_id;
  };

  for (EPLeaf &leaf : active_leaves) {
    assert(leaf.next && "Active leaf without a next node");

    bdd_node_id_t new_id    = translate_next_node(leaf.next->get_id());
    const BDDNode *new_node = new_bdd->get_node_by_id(new_id);
    assert(new_node && "New node not found in the new BDD.");

    leaf.next = new_node;
  }

  if (root) {
    root->visit_mutable_nodes([&new_bdd, translate_processed_node](EPNode *node) {
      Module *module = node->get_mutable_module();

      const BDDNode *node_bdd = module->get_node();
      bdd_node_id_t target_id = translate_processed_node(node_bdd->get_id());

      const BDDNode *new_node = new_bdd->get_node_by_id(target_id);
      assert(new_node && "BDDNode not found in the new BDD");

      module->set_node(new_node);

      return EPNodeVisitAction::Continue;
    });
  }

  meta.update_total_bdd_nodes(new_bdd.get());

  // Translating the symbols stored by the context.
  ctx.translate(new_bdd->get_mutable_symbol_manager(), translation_data.translated_symbols);

  // Replacing the BDD might change the hit rate estimations.
  ctx.get_profiler().clear_cache();

  // Translating Profiler symbols.
  ctx.get_mutable_profiler().translate(new_bdd->get_mutable_symbol_manager(), translation_data.reordered_node, translation_data.translated_symbols);

  // Reset the BDD only here, because we might lose the final reference to it and we needed the old nodes to find the new ones.
  bdd = std::move(new_bdd);

  sort_leaves();
}

void EP::debug() const {
  std::cerr << "\n";
  std::cerr << "ID: " << id << "\n";
  std::cerr << "Ancestors:";
  for (ep_id_t ancestor : ancestors) {
    std::cerr << "  " << ancestor;
  }
  std::cerr << "\n";
  debug_hit_rate();
  debug_placements();
  debug_active_leaves();
  debug_speculations();
  ctx.debug();
}

void EP::debug_placements() const {
  std::cerr << "Implementations:\n";
  const std::unordered_map<addr_t, DSImpl> &impls = ctx.get_ds_impls();
  for (const auto &[obj, impl] : impls) {
    std::cerr << "  " << obj << " -> " << impl << "\n";
  }
}

void EP::debug_hit_rate() const {
  const Profiler &profiler = ctx.get_profiler();
  profiler.debug();
}

void EP::debug_active_leaves() const {
  const Profiler &profiler = ctx.get_profiler();
  std::cerr << "Active leaves:\n";
  for (const EPLeaf &leaf : active_leaves) {
    assert(leaf.next && "Active leaf without a next node");
    std::cerr << "  " << leaf.next->dump(true, true);
    if (leaf.node) {
      std::cerr << " | " << leaf.node->dump();
      std::cerr << " | HR=" << profiler.get_hr(leaf.node) << "\n";
    }
  }
}

void EP::debug_speculations() const {
  if (cached_speculations.has_value()) {
    std::cerr << speculations2str(this, cached_speculations.value());
  }
}

void EP::assert_integrity() const {
  std::cerr << "***** Asserting integrity of EP " << id << " *****\n";
  std::vector<const EPNode *> nodes{root};

  while (nodes.size()) {
    const EPNode *node = nodes.back();
    nodes.pop_back();

    assert_or_panic(node, "Null node");
    assert_or_panic(node->get_module(), "BDDNode without a module");

    const Module *module    = node->get_module();
    const BDDNode *bdd_node = module->get_node();
    assert_or_panic(bdd_node, "Module without a node");

    const BDDNode *found_bdd_node = bdd->get_node_by_id(bdd_node->get_id());
    assert_or_panic(bdd_node == found_bdd_node, "BDDNode not found in the BDD");

    for (const EPNode *child : node->get_children()) {
      assert_or_panic(child, "Null child");
      assert_or_panic(child->get_prev() == node, "Child without the correct parent");
      nodes.push_back(child);
    }
  }

  for (const auto &[target, roots] : targets_roots) {
    for (const bdd_node_id_t root_id : roots) {
      const BDDNode *bdd_node = bdd->get_node_by_id(root_id);
      assert_or_panic(bdd_node, "Root node not found in the BDD");

      const BDDNode *found_bdd_node = bdd->get_node_by_id(bdd_node->get_id());
      assert_or_panic(bdd_node == found_bdd_node, "Root node not found in the BDD");
    }
  }

  for (const EPLeaf &leaf : active_leaves) {
    const BDDNode *next = leaf.next;
    assert_or_panic(next, "Active leaf without a next node");

    const BDDNode *found_next = bdd->get_node_by_id(next->get_id());
    assert_or_panic(next == found_next, "Next node not found in the BDD");
  }

  const BDD::inspection_report_t bdd_inspection_report = bdd->inspect();
  if (bdd_inspection_report.status != BDD::InspectionStatus::Ok) {
    panic("[EP=%lu] BDD inspection failed: %s", id, bdd_inspection_report.message.c_str());
  }
}

hit_rate_t EP::get_active_leaf_hit_rate() const {
  EPLeaf active_leaf = get_active_leaf();

  if (!active_leaf.node) {
    return 1_hr;
  }

  return ctx.get_profiler().get_hr(active_leaf.node);
}

void EP::sort_leaves() {
  TargetType initial_target = targets.get_initial_target().type;

  auto prioritize_switch_and_hot_paths = [this, initial_target](const EPLeaf &l1, const EPLeaf &l2) {
    // Only the first leaf may have no EPNode.
    assert(l1.node && "Leaf without a node");
    assert(l2.node && "Leaf without a node");

    if (l1.node->get_module()->get_next_target() != initial_target && l2.node->get_module()->get_next_target() == initial_target) {
      return false;
    }

    if (l1.node->get_module()->get_next_target() == initial_target && l2.node->get_module()->get_next_target() != initial_target) {
      return true;
    }

    if (l1.next == nullptr && l2.next == nullptr) {
      return true;
    }

    if (l1.next == nullptr && l2.next != nullptr) {
      return false;
    }

    if (l1.next != nullptr && l2.next == nullptr) {
      return true;
    }

    const hit_rate_t l1_hr = ctx.get_profiler().get_hr(l1.next);
    const hit_rate_t l2_hr = ctx.get_profiler().get_hr(l2.next);

    return l1_hr > l2_hr;
  };

  std::sort(active_leaves.begin(), active_leaves.end(), prioritize_switch_and_hot_paths);
}

std::list<const BDDNode *> EP::get_nodes_targeted_for_speculation() const {
  std::list<const BDDNode *> target_nodes;
  const TargetType initial_target = targets.get_initial_target().type;
  for (const EPLeaf &leaf : active_leaves) {
    if (leaf.node && leaf.node->get_module()->get_next_target() != initial_target) {
      continue;
    }

    assert_or_panic(leaf.next, "Active leaf without a next node");
    leaf.next->visit_nodes([this, &target_nodes](const BDDNode *node) {
      if (ctx.get_profiler().get_hr(node) == 0) {
        return BDDNodeVisitAction::Continue;
      }

      target_nodes.push_back(node);
      return BDDNodeVisitAction::Continue;
    });
  }

  return target_nodes;
}

EP::tput_cmp_t EP::compare_speculations_by_ignored_nodes(const spec_impl_t &old_speculation, const spec_impl_t &new_speculation, const BDDNode *node,
                                                         TargetType current_target, pps_t ingress,
                                                         const std::list<const BDDNode *> &speculation_target_nodes) const {
  std::list<const BDDNode *> old_speculation_target_nodes;
  std::list<const BDDNode *> new_speculation_target_nodes;

  const bdd_node_ids_t old_future_nodes = filter_away_nodes(new_speculation.skip, old_speculation.skip);
  for (const bdd_node_id_t old_future_node : old_future_nodes) {
    old_speculation_target_nodes.push_back(bdd->get_node_by_id(old_future_node));
  }

  const bdd_node_ids_t new_future_nodes = filter_away_nodes(old_speculation.skip, new_speculation.skip);
  for (const bdd_node_id_t new_future_node : new_future_nodes) {
    new_speculation_target_nodes.push_back(bdd->get_node_by_id(new_future_node));
  }

  old_speculation_target_nodes.remove(node);
  new_speculation_target_nodes.remove(node);

  old_speculation_target_nodes.remove_if([&speculation_target_nodes](const BDDNode *child) {
    return std::find(speculation_target_nodes.begin(), speculation_target_nodes.end(), child) == speculation_target_nodes.end();
  });

  new_speculation_target_nodes.remove_if([&speculation_target_nodes](const BDDNode *child) {
    return std::find(speculation_target_nodes.begin(), speculation_target_nodes.end(), child) == speculation_target_nodes.end();
  });

  const complete_speculation_t complete_speculation_peek_old =
      speculate(old_speculation.ctx, old_speculation_target_nodes, current_target, ingress, Lookahead::WithoutLookahead);
  const complete_speculation_t complete_speculation_peek_new =
      speculate(new_speculation.ctx, new_speculation_target_nodes, current_target, ingress, Lookahead::WithoutLookahead);

  const pps_t old_pps = complete_speculation_peek_old.final_ctx.get_perf_oracle().estimate_tput(ingress);
  const pps_t new_pps = complete_speculation_peek_new.final_ctx.get_perf_oracle().estimate_tput(ingress);

  return {.old_pps = old_pps, .new_pps = new_pps};
}

EP::tput_cmp_t EP::compare_speculations_with_reachable_nodes_lookahead(const spec_impl_t &old_speculation, const spec_impl_t &new_speculation,
                                                                       const BDDNode *node, TargetType current_target, pps_t ingress,
                                                                       const std::list<const BDDNode *> &speculation_target_nodes) const {
  std::list<const BDDNode *> old_speculation_target_nodes;
  std::list<const BDDNode *> new_speculation_target_nodes;

  for (const BDDNode *reachable_node : node->get_reachable_nodes()) {
    old_speculation_target_nodes.push_back(reachable_node);
    new_speculation_target_nodes.push_back(reachable_node);
  }

  old_speculation_target_nodes.remove_if([&old_speculation, node](const BDDNode *child) {
    return child == node || old_speculation.skip.find(child->get_id()) != old_speculation.skip.end();
  });

  new_speculation_target_nodes.remove_if([&new_speculation, node](const BDDNode *child) {
    return child == node || new_speculation.skip.find(child->get_id()) != new_speculation.skip.end();
  });

  const complete_speculation_t complete_speculation_peek_old =
      speculate(old_speculation.ctx, old_speculation_target_nodes, current_target, ingress, Lookahead::WithoutLookahead);
  const complete_speculation_t complete_speculation_peek_new =
      speculate(new_speculation.ctx, new_speculation_target_nodes, current_target, ingress, Lookahead::WithoutLookahead);

  const pps_t old_pps = complete_speculation_peek_old.final_ctx.get_perf_oracle().estimate_tput(ingress);
  const pps_t new_pps = complete_speculation_peek_new.final_ctx.get_perf_oracle().estimate_tput(ingress);

  return {.old_pps = old_pps, .new_pps = new_pps};
}

EP::tput_cmp_t EP::compare_speculations_with_unexplored_nodes_lookahead(const spec_impl_t &old_speculation, const spec_impl_t &new_speculation,
                                                                        const BDDNode *node, TargetType current_target, pps_t ingress,
                                                                        const std::list<const BDDNode *> &speculation_target_nodes) const {
  std::list<const BDDNode *> old_speculation_target_nodes;
  std::list<const BDDNode *> new_speculation_target_nodes;

  old_speculation_target_nodes = speculation_target_nodes;
  new_speculation_target_nodes = speculation_target_nodes;

  old_speculation_target_nodes.remove_if([&old_speculation, node](const BDDNode *child) {
    return child == node || old_speculation.skip.find(child->get_id()) != old_speculation.skip.end();
  });

  new_speculation_target_nodes.remove_if([&new_speculation, node](const BDDNode *child) {
    return child == node || new_speculation.skip.find(child->get_id()) != new_speculation.skip.end();
  });

  const complete_speculation_t complete_speculation_peek_old =
      speculate(old_speculation.ctx, old_speculation_target_nodes, current_target, ingress, Lookahead::WithoutLookahead);
  const complete_speculation_t complete_speculation_peek_new =
      speculate(new_speculation.ctx, new_speculation_target_nodes, current_target, ingress, Lookahead::WithoutLookahead);

  const pps_t old_pps = complete_speculation_peek_old.final_ctx.get_perf_oracle().estimate_tput(ingress);
  const pps_t new_pps = complete_speculation_peek_new.final_ctx.get_perf_oracle().estimate_tput(ingress);

  return {.old_pps = old_pps, .new_pps = new_pps};
}

// Compare the performance of an old speculation if it were subjected to the nodes ignored by the new speculation, and vise versa.
bool EP::is_better_speculation(const spec_impl_t &old_speculation, const spec_impl_t &new_speculation, const BDDNode *node, TargetType current_target,
                               pps_t ingress, const std::list<const BDDNode *> &speculation_target_nodes, Lookahead lookahead) const {
  if (ingress <= STABLE_TPUT_PRECISION) {
    return false;
  }

  const pps_t old_pps = old_speculation.ctx.get_perf_oracle().estimate_tput(ingress);
  if (old_pps <= STABLE_TPUT_PRECISION) {
    return true;
  }

  const pps_t new_pps = new_speculation.ctx.get_perf_oracle().estimate_tput(ingress);
  if (new_pps <= STABLE_TPUT_PRECISION) {
    return false;
  }

  tput_cmp_t tput_cmp =
      compare_speculations_by_ignored_nodes(old_speculation, new_speculation, node, current_target, ingress, speculation_target_nodes);

  if ((lookahead == Lookahead::WithoutLookahead) || (tput_cmp.old_pps != tput_cmp.new_pps)) {
    return tput_cmp.new_pps > tput_cmp.old_pps;
  }

  tput_cmp =
      compare_speculations_with_reachable_nodes_lookahead(old_speculation, new_speculation, node, current_target, ingress, speculation_target_nodes);

  if (tput_cmp.old_pps != tput_cmp.new_pps) {
    return tput_cmp.new_pps > tput_cmp.old_pps;
  }

  tput_cmp =
      compare_speculations_with_unexplored_nodes_lookahead(old_speculation, new_speculation, node, current_target, ingress, speculation_target_nodes);

  return tput_cmp.new_pps > tput_cmp.old_pps;
}

spec_impl_t EP::get_best_speculation(const BDDNode *node, TargetType current_target, const Context &spec_ctx, pps_t ingress,
                                     const std::list<const BDDNode *> &speculation_target_nodes, Lookahead lookahead) const {
  std::optional<spec_impl_t> best;

  for (const TargetView &target : targets.elements) {
    if (target.type != current_target) {
      continue;
    }

    for (const ModuleFactory *modgen : target.module_factories) {
      std::optional<spec_impl_t> spec = modgen->speculate(this, node, spec_ctx);

      if (!spec.has_value()) {
        continue;
      }

      // if (id == 47 && node->get_id() == 46) {
      //   std::cerr << "Speculation for " << node->dump(true) << "\n";
      //   std::cerr << "  " << spec2str(*spec, bdd.get()) << "\n";
      //   if (best.has_value()) {
      //     auto is_better = is_better_speculation(*best, *spec, node, current_target, ingress, speculation_target_nodes);
      //     std::cerr << "  is better than " << spec2str(*best, bdd.get()) << "? " << is_better << "\n";
      //   }
      // }

      if (!best.has_value()) {
        best = *spec;
        continue;
      }

      assert(best.has_value() && "No best speculation");
      assert(spec.has_value() && "No speculation");

      if (is_better_speculation(*best, *spec, node, current_target, ingress, speculation_target_nodes, lookahead)) {
        best = *spec;
      }
    }
  }

  if (!best.has_value()) {
    spec_ctx.debug();
    // EPViz::visualize(this, false);
    // BDDViz::visualize(bdd.get(), false);

    panic("No module to speculative execute\n"
          "EP:     %lu\n"
          "Target: %s\n"
          "BDDNode:   %s",
          id, to_string(current_target).c_str(), node->dump(true).c_str());
  }

  return *best;
}

EP::complete_speculation_t EP::speculate(const Context &current_ctx, std::list<const BDDNode *> speculation_target_nodes, TargetType target,
                                         pps_t ingress, Lookahead lookahead) const {
  complete_speculation_t complete_speculation = {
      .speculations_per_node = {},
      .final_ctx             = current_ctx,
  };

  while (!speculation_target_nodes.empty()) {
    const BDDNode *node = speculation_target_nodes.front();
    speculation_target_nodes.pop_front();

    const spec_impl_t speculation = get_best_speculation(node, target, complete_speculation.final_ctx, ingress, speculation_target_nodes, lookahead);

    complete_speculation.speculations_per_node.push_back(speculation);
    complete_speculation.final_ctx = speculation.ctx;

    for (const bdd_node_id_t bdd_node_id_to_skip : speculation.skip) {
      speculation_target_nodes.remove(bdd->get_node_by_id(bdd_node_id_to_skip));
    }

    if (speculation.next_target.has_value() && speculation.next_target != target) {
      // Just ignore if we change the target, we only care about the switch nodes for now.
      for (const BDDNode *reachable_node : node->get_reachable_nodes()) {
        speculation_target_nodes.remove(reachable_node);
      }
    }
  }

  return complete_speculation;
}

pps_t EP::speculate_tput_pps() const {
  if (cached_tput_speculation.has_value()) {
    return *cached_tput_speculation;
  }

  const std::list<const BDDNode *> &speculation_target_nodes = get_nodes_targeted_for_speculation();
  const TargetType initial_target                            = targets.get_initial_target().type;
  const pps_t ingress                                        = estimate_tput_pps();

  const complete_speculation_t complete_speculation = speculate(ctx, speculation_target_nodes, initial_target, ingress, Lookahead::WithLookahead);

  auto egress_estimation_from_ingress = [&complete_speculation](pps_t tput) -> tput_estimation_t {
    const PerfOracle &perf_oracle = complete_speculation.final_ctx.get_perf_oracle();

    const tput_estimation_t estimation = {
        .ingress           = tput,
        .egress_estimation = perf_oracle.estimate_tput(tput),
        .unavoidable_drop  = static_cast<pps_t>(tput * perf_oracle.get_dropped_ingress().value),
    };

    return estimation;
  };

  const pps_t egress = find_stable_tput(ingress, egress_estimation_from_ingress);

  cached_tput_speculation = egress;
  cached_speculations     = complete_speculation.speculations_per_node;

  // if (id == 47 || id == 54) {
  //   std::cerr << speculations2str(this, speculations);
  //   spec_ctx.debug();
  //   std::cerr << "Ingress: " << tput2str(pps2bps(ingress, ctx.get_profiler().get_avg_pkt_bytes()), "bps", true) << "\n";
  //   std::cerr << "Egress from ingress: "
  //             << tput2str(pps2bps(egress_estimation_from_ingress(ingress).egress_estimation, ctx.get_profiler().get_avg_pkt_bytes()), "bps", true)
  //             << "\n";
  //   std::cerr << "Stable egress: " << tput2str(pps2bps(egress, ctx.get_profiler().get_avg_pkt_bytes()), "bps", true) << "\n";
  //   // BDDViz::visualize(bdd.get(), false);
  //   // EPViz::visualize(this, false);
  //   // ProfilerViz::visualize(bdd.get(), spec_ctx.get_profiler(), false);
  //   // debug_active_leaves();
  //   dbg_pause();
  // }

  return egress;
}

pps_t EP::estimate_tput_pps() const {
  if (cached_tput_estimation.has_value()) {
    return *cached_tput_estimation;
  }

  const pps_t max_ingress = ctx.get_perf_oracle().get_max_input_pps();

  auto egress_estimation_from_ingress = [this](pps_t tput) {
    const PerfOracle &perf_oracle = ctx.get_perf_oracle();

    const tput_estimation_t estimation = {
        .ingress           = tput,
        .egress_estimation = perf_oracle.estimate_tput(tput),
        .unavoidable_drop  = static_cast<pps_t>(tput * perf_oracle.get_dropped_ingress().value),
    };

    return estimation;
  };

  const pps_t egress     = find_stable_tput(max_ingress, egress_estimation_from_ingress);
  cached_tput_estimation = egress;

  return egress;
}

port_ingress_t EP::get_node_egress(hit_rate_t hr, std::vector<int> past_recirculations) const {
  port_ingress_t egress;

  if (past_recirculations.empty()) {
    egress.global = hr;
  } else {
    const int rport = past_recirculations[0];
    int depth       = 1;

    for (size_t i = 1; i < past_recirculations.size(); i++) {
      if (past_recirculations[i] == rport) {
        depth++;
      } else {
        break;
      }
    }

    egress.recirc[{rport, depth}] = hr;
  }

  return egress;
}

port_ingress_t EP::get_node_egress(hit_rate_t hr, const EPNode *node) const {
  port_ingress_t egress;
  if (node->get_module()->get_target() == TargetType::Controller) {
    egress.controller = hr;
    return egress;
  }

  std::vector<u16> past_recirculations = node->get_past_recirculations();

  if (past_recirculations.empty()) {
    egress.global = hr;
  } else {
    const u16 rport = past_recirculations[0];
    int depth       = 1;

    for (size_t i = 1; i < past_recirculations.size(); i++) {
      if (past_recirculations[i] == rport) {
        depth++;
      } else {
        break;
      }
    }

    egress.recirc[{rport, depth}] = hr;
  }

  return egress;
}

void EP::clear_caches() const {
  cached_speculations.reset();
  cached_tput_estimation.reset();
  cached_tput_speculation.reset();
}

} // namespace LibSynapse