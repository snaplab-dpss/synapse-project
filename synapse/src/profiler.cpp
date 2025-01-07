#include <iomanip>

#include "profiler.h"
#include "execution_plan/execution_plan.h"
#include "random_engine.h"
#include "log.h"
#include "exprs/exprs.h"
#include "exprs/solver.h"

namespace synapse {
namespace {
hit_rate_t clamp_fraction(hit_rate_t fraction) { return std::min(1.0, std::max(0.0, fraction)); }

ProfilerNode *build_profiler_tree(const Node *node, const bdd_profile_t *bdd_profile,
                                  u64 max_count) {
  ProfilerNode *result = nullptr;

  if (!node) {
    return nullptr;
  }

  node->visit_nodes([&bdd_profile, max_count, &result](const Node *node) {
    if (node->get_type() != NodeType::Branch) {
      return NodeVisitAction::Continue;
    }

    const Branch *branch = dynamic_cast<const Branch *>(node);

    klee::ref<klee::Expr> condition = branch->get_condition();
    u64 counter = bdd_profile->counters.at(node->get_id());
    hit_rate_t fraction = clamp_fraction(static_cast<hit_rate_t>(counter) / max_count);
    node_id_t bdd_node_id = node->get_id();

    ProfilerNode *new_node = new ProfilerNode(condition, fraction, bdd_node_id);

    const Node *on_true = branch->get_on_true();
    const Node *on_false = branch->get_on_false();

    new_node->on_true = build_profiler_tree(on_true, bdd_profile, max_count);
    new_node->on_false = build_profiler_tree(on_false, bdd_profile, max_count);

    if (!new_node->on_true && on_true) {
      u64 on_true_counter = bdd_profile->counters.at(on_true->get_id());
      hit_rate_t on_true_fraction =
          clamp_fraction(static_cast<hit_rate_t>(on_true_counter) / max_count);
      node_id_t bdd_node_id = on_true->get_id();
      new_node->on_true = new ProfilerNode(nullptr, on_true_fraction, bdd_node_id);
    }

    if (!new_node->on_false && on_false) {
      u64 on_false_counter = bdd_profile->counters.at(on_false->get_id());
      hit_rate_t on_false_fraction =
          clamp_fraction(static_cast<hit_rate_t>(on_false_counter) / max_count);
      node_id_t bdd_node_id = on_false->get_id();
      new_node->on_false = new ProfilerNode(nullptr, on_false_fraction, bdd_node_id);
    }

    if (new_node->on_true) {
      new_node->on_true->prev = new_node;
    }

    if (new_node->on_false) {
      new_node->on_false->prev = new_node;
    }

    result = new_node;
    return NodeVisitAction::Stop;
  });

  return result;
}

bdd_profile_t build_random_bdd_profile(const BDD *bdd) {
  bdd_profile_t bdd_profile;

  bdd_profile.meta.pkts = 100'000;
  bdd_profile.meta.bytes = bdd_profile.meta.pkts * std::max(64, RandomEngine::generate() % 1500);

  const Node *root = bdd->get_root();
  bdd_profile.counters[root->get_id()] = bdd_profile.meta.pkts;

  root->visit_nodes([&bdd_profile](const Node *node) {
    SYNAPSE_ASSERT(bdd_profile.counters.find(node->get_id()) != bdd_profile.counters.end(),
                   "Node counter not found");
    u64 current_counter = bdd_profile.counters[node->get_id()];

    switch (node->get_type()) {
    case NodeType::Branch: {
      const Branch *branch = dynamic_cast<const Branch *>(node);

      const Node *on_true = branch->get_on_true();
      const Node *on_false = branch->get_on_false();

      SYNAPSE_ASSERT(on_true, "Branch node without on_true");
      SYNAPSE_ASSERT(on_false, "Branch node without on_false");

      u64 on_true_counter = RandomEngine::generate() % (current_counter + 1);
      u64 on_false_counter = current_counter - on_true_counter;

      bdd_profile.counters[on_true->get_id()] = on_true_counter;
      bdd_profile.counters[on_false->get_id()] = on_false_counter;
    } break;
    case NodeType::Call: {
      const Call *call_node = dynamic_cast<const Call *>(node);
      const call_t &call = call_node->get_call();

      if (call.function_name == "map_get" || call.function_name == "map_put" ||
          call.function_name == "map_erase") {
        klee::ref<klee::Expr> map_addr = call.args.at("map").expr;

        bdd_profile_t::map_stats_t::node_t map_stats;
        map_stats.node = node->get_id();
        map_stats.pkts = current_counter;
        map_stats.flows = std::max(1ul, RandomEngine::generate() % current_counter);

        u64 avg_pkts_per_flow = current_counter / map_stats.flows;
        for (u64 i = 0; i < map_stats.flows; i++) {
          map_stats.pkts_per_flow.push_back(avg_pkts_per_flow);
        }

        bdd_profile.stats_per_map[expr_addr_to_obj_addr(map_addr)].nodes.push_back(map_stats);
      }

      if (node->get_next()) {
        const Node *next = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }
    } break;
    case NodeType::Route: {
      if (node->get_next()) {
        const Node *next = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }
    } break;
    }

    return NodeVisitAction::Continue;
  });

  return bdd_profile;
}

void recursive_update_fractions(ProfilerNode *node, hit_rate_t parent_old_fraction,
                                hit_rate_t parent_new_fraction) {
  if (!node) {
    return;
  }

  SYNAPSE_ASSERT(parent_old_fraction >= 0.0, "Invalid parent old fraction");
  SYNAPSE_ASSERT(parent_old_fraction <= 1.0, "Invalid parent old fraction");

  SYNAPSE_ASSERT(parent_new_fraction >= 0.0, "Invalid parent new fraction");
  SYNAPSE_ASSERT(parent_new_fraction <= 1.0, "Invalid parent new fraction");

  hit_rate_t old_fraction = clamp_fraction(node->fraction);
  hit_rate_t new_fraction = clamp_fraction(
      parent_old_fraction != 0 ? (parent_new_fraction / parent_old_fraction) * node->fraction : 0);

  node->fraction = new_fraction;

  recursive_update_fractions(node->on_true, old_fraction, new_fraction);
  recursive_update_fractions(node->on_false, old_fraction, new_fraction);
}
} // namespace

ProfilerNode::ProfilerNode(klee::ref<klee::Expr> _constraint, hit_rate_t _fraction)
    : constraint(_constraint), fraction(_fraction), on_true(nullptr), on_false(nullptr),
      prev(nullptr) {}

ProfilerNode::ProfilerNode(klee::ref<klee::Expr> _constraint, hit_rate_t _fraction,
                           node_id_t _node_id)
    : constraint(_constraint), fraction(_fraction), bdd_node_id(_node_id), on_true(nullptr),
      on_false(nullptr), prev(nullptr) {}

ProfilerNode::~ProfilerNode() {
  if (on_true) {
    delete on_true;
    on_true = nullptr;
  }

  if (on_false) {
    delete on_false;
    on_false = nullptr;
  }
}

ProfilerNode *ProfilerNode::clone(bool keep_bdd_info) const {
  ProfilerNode *new_node = new ProfilerNode(constraint, fraction);

  if (keep_bdd_info) {
    new_node->bdd_node_id = bdd_node_id;
  }

  new_node->flows_stats = flows_stats;

  if (on_true) {
    new_node->on_true = on_true->clone(keep_bdd_info);
    new_node->on_true->prev = new_node;
  }

  if (on_false) {
    new_node->on_false = on_false->clone(keep_bdd_info);
    new_node->on_false->prev = new_node;
  }

  return new_node;
}

std::ostream &operator<<(std::ostream &os, const ProfilerNode &node) {
  os << "<";
  os << "fraction=" << node.fraction;

  if (!node.constraint.isNull()) {
    os << ", ";
    os << "cond=" << pretty_print_expr(node.constraint, true);
  }

  if (node.bdd_node_id) {
    os << ", ";
    os << "bdd_node=" << *node.bdd_node_id;
  }

  os << ">";

  return os;
}

void ProfilerNode::debug(int lvl) const {
  Log::dbg() << *this;
  Log::dbg() << "\n";

  if (on_true) {
    lvl++;

    // Log::dbg() << std::string(2 * lvl, '|');
    for (int i = 0; i < 2 * lvl; i++)
      Log::dbg() << ((i % 2 != 0) ? "|" : " ");

    Log::dbg() << "[T] ";
    on_true->debug(lvl);
    lvl--;
  }

  if (on_false) {
    lvl++;

    for (int i = 0; i < 2 * lvl; i++)
      Log::dbg() << ((i % 2 != 0) ? "|" : " ");
    // Log::dbg() << std::string(2 * lvl, '|');

    Log::dbg() << "[F] ";
    on_false->debug(lvl);
    lvl--;
  }
}

FlowStats ProfilerNode::get_flow_stats(klee::ref<klee::Expr> flow_id) const {
  for (const FlowStats &stats : flows_stats) {
    if (solver_toolbox.are_exprs_always_equal(flow_id, stats.flow_id)) {
      return stats;
    }
  }

  SYNAPSE_PANIC("Flow stats not found");
}

Profiler::Profiler(const BDD *bdd, const bdd_profile_t &_bdd_profile)
    : bdd_profile(new bdd_profile_t(_bdd_profile)), root(nullptr),
      avg_pkt_size(bdd_profile->meta.bytes / bdd_profile->meta.pkts), cache() {
  const Node *bdd_root = bdd->get_root();

  SYNAPSE_ASSERT(bdd_profile->counters.find(bdd_root->get_id()) != bdd_profile->counters.end(),
                 "Root node not found");
  u64 max_count = bdd_profile->counters.at(bdd_root->get_id());

  root = std::shared_ptr<ProfilerNode>(build_profiler_tree(bdd_root, bdd_profile.get(), max_count));

  for (const auto &[map_addr, map_stats] : bdd_profile->stats_per_map) {
    for (const auto &node_map_stats : map_stats.nodes) {
      const Node *node = bdd->get_node_by_id(node_map_stats.node);
      SYNAPSE_ASSERT(node, "Node not found");

      constraints_t constraints = node->get_ordered_branch_constraints();
      ProfilerNode *profiler_node = get_node(constraints);
      SYNAPSE_ASSERT(profiler_node, "Profiler node not found");

      SYNAPSE_ASSERT(node->get_type() == NodeType::Call, "Invalid node type");
      const Call *call_node = dynamic_cast<const Call *>(node);
      const call_t &call = call_node->get_call();

      SYNAPSE_ASSERT(call.function_name == "map_get" || call.function_name == "map_put" ||
                         call.function_name == "map_erase",
                     "Invalid call");

      SYNAPSE_ASSERT(call.args.find("key") != call.args.end(), "Key not found");
      klee::ref<klee::Expr> flow_id = call.args.at("key").in;

      FlowStats flow_stats = {
          .flow_id = flow_id,
          .pkts = node_map_stats.pkts,
          .flows = node_map_stats.flows,
          .pkts_per_flow = node_map_stats.pkts_per_flow,
      };

      profiler_node->flows_stats.push_back(flow_stats);
    }
  }
}

Profiler::Profiler(const BDD *bdd) : Profiler(bdd, build_random_bdd_profile(bdd)) {}

Profiler::Profiler(const BDD *bdd, const std::string &bdd_profile_fname)
    : Profiler(bdd, parse_bdd_profile(bdd_profile_fname)) {}

Profiler::Profiler(const Profiler &other)
    : bdd_profile(other.bdd_profile), root(other.root), avg_pkt_size(other.avg_pkt_size),
      cache(other.cache) {}

Profiler::Profiler(Profiler &&other)
    : bdd_profile(std::move(other.bdd_profile)), root(std::move(other.root)),
      avg_pkt_size(other.avg_pkt_size), cache(std::move(other.cache)) {
  other.root = nullptr;
}

Profiler &Profiler::operator=(const Profiler &other) {
  if (this == &other) {
    return *this;
  }

  bdd_profile = other.bdd_profile;
  root = other.root;
  avg_pkt_size = other.avg_pkt_size;
  cache = other.cache;

  return *this;
}

const bdd_profile_t *Profiler::get_bdd_profile() const { return bdd_profile.get(); }

bytes_t Profiler::get_avg_pkt_bytes() const { return avg_pkt_size; }

ProfilerNode *Profiler::get_node(const constraints_t &constraints) const {
  ProfilerNode *current = root.get();

  for (klee::ref<klee::Expr> cnstr : constraints) {
    if (!current) {
      return nullptr;
    }

    SYNAPSE_ASSERT(!current->constraint.isNull(), "Invalid profiler node");

    klee::ConstraintManager manager;
    manager.addConstraint(current->constraint);

    bool always_true = solver_toolbox.is_expr_always_true(manager, cnstr);
    bool always_false = solver_toolbox.is_expr_always_false(manager, cnstr);

    SYNAPSE_ASSERT(always_true || always_false, "Invalid profiler node");

    if (always_true) {
      current = current->on_true;
    } else {
      current = current->on_false;
    }
  }

  SYNAPSE_ASSERT(current, "Profiler node not found");
  return current;
}

void Profiler::replace_root(klee::ref<klee::Expr> constraint, hit_rate_t fraction) {
  SYNAPSE_ASSERT(false, "Attempted to replace Profiler root node");

  ProfilerNode *new_node = new ProfilerNode(constraint, 1.0);

  ProfilerNode *node = root.get();
  root = std::shared_ptr<ProfilerNode>(new_node);

  new_node->on_true = node;
  new_node->on_false = node->clone(false);

  new_node->on_true->prev = new_node;
  new_node->on_false->prev = new_node;

  hit_rate_t fraction_on_true = clamp_fraction(fraction);
  hit_rate_t fraction_on_false = clamp_fraction(new_node->fraction - fraction);

  SYNAPSE_ASSERT(fraction_on_true <= new_node->fraction, "Invalid fraction");
  SYNAPSE_ASSERT(fraction_on_false <= new_node->fraction, "Invalid fraction");

  recursive_update_fractions(new_node->on_true, new_node->fraction, fraction_on_true);
  recursive_update_fractions(new_node->on_false, new_node->fraction, fraction_on_false);
}

void Profiler::append(ProfilerNode *node, klee::ref<klee::Expr> constraint, hit_rate_t fraction) {
  ProfilerNode *parent = node->prev;

  if (!parent) {
    replace_root(constraint, fraction);
    return;
  }

  ProfilerNode *new_node = new ProfilerNode(constraint, node->fraction);

  SYNAPSE_ASSERT((parent->on_true == node) || (parent->on_false == node), "Invalid node");
  if (parent->on_true == node) {
    parent->on_true = new_node;
  } else {
    parent->on_false = new_node;
  }

  new_node->prev = parent;

  new_node->on_true = node;
  new_node->on_true->prev = new_node;

  new_node->on_false = node->clone(false);
  new_node->on_false->prev = new_node;

  hit_rate_t fraction_on_true = clamp_fraction(fraction);
  hit_rate_t fraction_on_false = clamp_fraction(new_node->fraction - fraction);

  recursive_update_fractions(new_node->on_true, new_node->fraction, fraction_on_true);
  recursive_update_fractions(new_node->on_false, new_node->fraction, fraction_on_false);
}

Profiler::family_t Profiler::get_family(ProfilerNode *node) const {
  family_t family = {
      .node = node,
      .parent = node->prev,
      .grandparent = nullptr,
      .sibling = nullptr,
  };

  if (family.parent) {
    family.grandparent = family.parent->prev;
    family.sibling =
        (family.parent->on_true == node) ? family.parent->on_false : family.parent->on_true;
  }

  return family;
}

void Profiler::remove(ProfilerNode *node) {
  family_t family = get_family(node);

  SYNAPSE_ASSERT(family.parent, "Cannot remove the root node");
  hit_rate_t parent_fraction = family.parent->fraction;

  // By removing the current node, the parent is no longer needed (its purpose
  // was to differentiate between the on_true and on_false nodes, but now only
  // one side is left).

  SYNAPSE_ASSERT(family.grandparent, "Cannot remove the root node");

  if (family.grandparent->on_true == family.parent) {
    family.grandparent->on_true = family.sibling;
  } else {
    family.grandparent->on_false = family.sibling;
  }

  family.sibling->prev = family.grandparent;

  family.parent->on_true = nullptr;
  family.parent->on_false = nullptr;
  delete family.parent;

  node->on_true = nullptr;
  node->on_false = nullptr;
  delete node;

  hit_rate_t old_fraction = family.sibling->fraction;
  hit_rate_t new_fraction = parent_fraction;

  family.sibling->fraction = new_fraction;

  recursive_update_fractions(family.sibling->on_true, old_fraction, new_fraction);
  recursive_update_fractions(family.sibling->on_false, old_fraction, new_fraction);
}

void Profiler::remove(const constraints_t &constraints) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);
  remove(node);
}

void Profiler::insert_relative(const constraints_t &constraints, klee::ref<klee::Expr> constraint,
                               hit_rate_t rel_fraction_on_true) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);
  hit_rate_t fraction = rel_fraction_on_true * node->fraction;
  append(node, constraint, fraction);
}

void Profiler::scale(const constraints_t &constraints, hit_rate_t factor) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);

  hit_rate_t old_fraction = clamp_fraction(node->fraction);
  hit_rate_t new_fraction = clamp_fraction(node->fraction * factor);

  node->fraction = new_fraction;

  recursive_update_fractions(node->on_true, old_fraction, new_fraction);
  recursive_update_fractions(node->on_false, old_fraction, new_fraction);
}

void Profiler::set(const constraints_t &constraints, hit_rate_t new_hr) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);

  hit_rate_t old_fraction = clamp_fraction(node->fraction);
  hit_rate_t new_fraction = clamp_fraction(new_hr);

  node->fraction = new_fraction;

  recursive_update_fractions(node->on_true, old_fraction, new_fraction);
  recursive_update_fractions(node->on_false, old_fraction, new_fraction);
}

hit_rate_t Profiler::get_hr(const Node *node) const {
  auto found_it = cache.n2p.find(node->get_id());
  if (found_it != cache.n2p.end()) {
    SYNAPSE_ASSERT(found_it->second, "Invalid profiler node");
    return found_it->second->fraction;
  }

  constraints_t constraints = node->get_ordered_branch_constraints();
  ProfilerNode *profiler_node = get_node(constraints);

  if (!profiler_node) {
    SYNAPSE_PANIC("Profiler node not found");
  }

  cache.n2p[node->get_id()] = profiler_node;
  cache.p2n[profiler_node] = node->get_id();

  return profiler_node->fraction;
}

hit_rate_t Profiler::get_hr(const EPNode *node) const {
  auto found_it = cache.e2p.find(node->get_id());
  if (found_it != cache.e2p.end()) {
    SYNAPSE_ASSERT(found_it->second, "Invalid profiler node");
    return found_it->second->fraction;
  }

  constraints_t constraints = node->get_constraints();
  ProfilerNode *profiler_node = get_node(constraints);

  if (!profiler_node) {
    SYNAPSE_PANIC("Profiler node not found");
  }

  cache.e2p[node->get_id()] = profiler_node;
  cache.p2e[profiler_node] = node->get_id();

  return profiler_node->fraction;
}

hit_rate_t Profiler::get_hr(const constraints_t &constraints) const {
  ProfilerNode *node = get_node(constraints);
  return node->fraction;
}

void Profiler::clear_cache() const {
  cache.n2p.clear();
  cache.p2n.clear();
  cache.e2p.clear();
  cache.p2e.clear();
}

void Profiler::debug() const {
  Log::dbg() << "\n============== Hit Rate Tree ==============\n";
  if (root) {
    root->debug();
  }
  Log::dbg() << "===========================================\n\n";
}

FlowStats Profiler::get_flow_stats(const constraints_t &constraints,
                                   klee::ref<klee::Expr> flow_id) const {
  return get_node(constraints)->get_flow_stats(flow_id);
}

void Profiler::clone_tree_if_shared() {
  clear_cache();

  SYNAPSE_ASSERT(root, "Invalid profiler root node");

  // No need to clone a tree that is not shared.
  if (root.use_count() == 1) {
    return;
  }

  root = std::shared_ptr<ProfilerNode>(root->clone(true));
}

void Profiler::replace_constraint(ProfilerNode *node, klee::ref<klee::Expr> constraint) {
  clone_tree_if_shared();
  node->constraint = constraint;
}

void Profiler::replace_constraint(const constraints_t &constraints,
                                  klee::ref<klee::Expr> constraint) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);
  replace_constraint(node, constraint);
}
} // namespace synapse