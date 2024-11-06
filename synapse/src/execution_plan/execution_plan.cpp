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
  assert(targets.size());
  std::unordered_set<TargetType> targets_types;

  for (const Target *target : targets) {
    targets_types.insert(target->type);
  }

  return targets_types;
}

static TargetType get_initial_target(const targets_t &targets) {
  assert(targets.size());
  return targets[0]->type;
}

EP::EP(std::shared_ptr<const BDD> _bdd, const targets_t &_targets,
       const toml::table &_config, const Profiler &_profiler)
    : id(counter++), bdd(_bdd), root(nullptr),
      initial_target(get_initial_target(_targets)), targets(_targets),
      ctx(_bdd.get(), _targets, initial_target, _config, _profiler),
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
    assert(other.active_leaves.size() == 1);
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

const std::vector<EPLeaf> &EP::get_active_leaves() const {
  return active_leaves;
}

const targets_t &EP::get_targets() const { return targets; }

const nodes_t &EP::get_target_roots(TargetType target) const {
  assert(targets_roots.find(target) != targets_roots.end() &&
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
    assert(module);

    if (types.find(module->get_type()) != types.end()) {
      found.push_back(node);
    }

    return EPNodeVisitAction::VISIT_CHILDREN;
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
  assert(!active_leaves.empty() && "No active leaf");
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

  assert(has_active_leaf() && "No active leaf");
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
    assert(leaf.next);

    node_id_t new_id = translate_next_node(leaf.next->get_id());
    const Node *new_node = new_bdd->get_node_by_id(new_id);
    assert(new_node && "New node not found in the new BDD.");

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
    assert(new_node && "New node not found in the new BDD.");

    module->set_node(new_node);

    return EPNodeVisitAction::VISIT_CHILDREN;
  });

  meta.update_total_bdd_nodes(new_bdd);

  // Replacing the BDD might change the hit rate estimations.
  ctx.get_profiler().clear_cache();

  // Reset the BDD only here, because we might lose the final reference to it
  // and we needed the old nodes to find the new ones.
  bdd.reset(new_bdd);

  sort_leaves();
}

void EP::visit(EPVisitor &visitor) const { visitor.visit(this); }

void EP::debug() const {
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
    assert(leaf.next);
    Log::dbg() << "  " << leaf.next->dump(true, true);
    if (leaf.node) {
      Log::dbg() << " | " << leaf.node->dump();
      Log::dbg() << " | HR=" << profiler.get_hr(leaf.node) << "\n";
    }
  }
}

void EP::inspect() const {
  if (!Log::is_debug_active()) {
    return;
  }

  std::vector<const EPNode *> nodes{root};

  while (nodes.size()) {
    const EPNode *node = nodes.back();
    nodes.pop_back();

    assert(node);
    assert(node->get_module());

    const Module *module = node->get_module();
    const Node *bdd_node = module->get_node();
    assert(bdd_node);

    const Node *found_bdd_node = bdd->get_node_by_id(bdd_node->get_id());
    assert(bdd_node == found_bdd_node);

    for (const EPNode *child : node->get_children()) {
      assert(child);
      assert(child->get_prev() == node);
      nodes.push_back(child);
    }
  }

  for (const auto &[target, roots] : targets_roots) {
    for (const node_id_t root_id : roots) {
      const Node *bdd_node = bdd->get_node_by_id(root_id);
      assert(bdd_node);

      const Node *found_bdd_node = bdd->get_node_by_id(bdd_node->get_id());
      assert(bdd_node == found_bdd_node);
    }
  }

  for (const EPLeaf &leaf : active_leaves) {
    const Node *next = leaf.next;
    assert(next);

    const Node *found_next = bdd->get_node_by_id(next->get_id());
    assert(next == found_next);
  }

  bdd->inspect();
}

hit_rate_t EP::get_active_leaf_hit_rate() const {
  EPLeaf active_leaf = get_active_leaf();

  if (!active_leaf.node) {
    return 1;
  }

  return ctx.get_profiler().get_hr(active_leaf.node);
}

void EP::add_hit_rate_estimation(klee::ref<klee::Expr> new_constraint,
                                 hit_rate_t estimation_rel) {
  EPLeaf active_leaf = get_active_leaf();
  assert(active_leaf.node);
  constraints_t constraints = active_leaf.node->get_constraints();
  ctx.add_hit_rate_estimation(constraints, new_constraint, estimation_rel);
}

void EP::remove_hit_rate_node(const constraints_t &constraints) {
  ctx.remove_hit_rate_node(constraints);
}

void EP::sort_leaves() {
  auto prioritize_switch_and_hot_paths = [this](const EPLeaf &l1,
                                                const EPLeaf &l2) {
    // Only the first leaf may have no EPNode.
    assert(l1.node);
    assert(l2.node);

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