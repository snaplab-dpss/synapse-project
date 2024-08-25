#include <iomanip>

#include "profiler.h"
#include "random_engine.h"
#include "log.h"
#include "exprs/exprs.h"
#include "exprs/solver.h"

static hit_rate_t normalize_fraction(hit_rate_t fraction) {
  return std::min(1.0, std::max(0.0, fraction));
}

ProfilerNode::ProfilerNode(klee::ref<klee::Expr> _constraint,
                           hit_rate_t _fraction)
    : constraint(_constraint), fraction(_fraction), on_true(nullptr),
      on_false(nullptr), prev(nullptr) {}

ProfilerNode::ProfilerNode(klee::ref<klee::Expr> _constraint,
                           hit_rate_t _fraction, node_id_t _bdd_node_id)
    : constraint(_constraint), fraction(_fraction), bdd_node_id(_bdd_node_id),
      on_true(nullptr), on_false(nullptr), prev(nullptr) {}

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
  if (node.bdd_node_id) {
    os << ", ";
    os << "bdd_node=" << *node.bdd_node_id;
  } else {
    if (!node.constraint.isNull()) {
      os << ", ";
      os << "cond=" << expr_to_string(node.constraint, true);
    }
  }
  for (const FlowStats &flow_stats : node.flows_stats) {
    os << ", ";
    os << "#flows=" << flow_stats.total_flows;
  }
  os << ">";

  return os;
}

void ProfilerNode::log_debug(int lvl) const {
  Log::dbg() << *this;
  Log::dbg() << "\n";

  if (on_true) {
    lvl++;

    // Log::dbg() << std::string(2 * lvl, '|');
    for (int i = 0; i < 2 * lvl; i++)
      Log::dbg() << ((i % 2 != 0) ? "|" : " ");

    Log::dbg() << "[T] ";
    on_true->log_debug(lvl);
    lvl--;
  }

  if (on_false) {
    lvl++;

    for (int i = 0; i < 2 * lvl; i++)
      Log::dbg() << ((i % 2 != 0) ? "|" : " ");
    // Log::dbg() << std::string(2 * lvl, '|');

    Log::dbg() << "[F] ";
    on_false->log_debug(lvl);
    lvl--;
  }
}

bool ProfilerNode::get_flow_stats(klee::ref<klee::Expr> flow_id,
                                  FlowStats &flow_stats) const {
  for (const FlowStats &stats : flows_stats) {
    if (solver_toolbox.are_exprs_always_equal(flow_id, stats.flow_id)) {
      flow_stats = stats;
      return true;
    }
  }

  return false;
}

static ProfilerNode *build_profiler_tree(const Node *node,
                                         const bdd_profile_t &bdd_profile,
                                         u64 max_count) {
  ProfilerNode *result = nullptr;

  if (!node) {
    return nullptr;
  }

  node->visit_nodes([&bdd_profile, max_count, &result](const Node *node) {
    if (node->get_type() != NodeType::BRANCH) {
      return NodeVisitAction::VISIT_CHILDREN;
    }

    const Branch *branch = static_cast<const Branch *>(node);

    klee::ref<klee::Expr> condition = branch->get_condition();
    u64 counter = bdd_profile.counters.at(node->get_id());
    hit_rate_t fraction =
        normalize_fraction(static_cast<hit_rate_t>(counter) / max_count);
    node_id_t bdd_node_id = node->get_id();

    ProfilerNode *new_node = new ProfilerNode(condition, fraction, bdd_node_id);

    const Node *on_true = branch->get_on_true();
    const Node *on_false = branch->get_on_false();

    new_node->on_true = build_profiler_tree(on_true, bdd_profile, max_count);
    new_node->on_false = build_profiler_tree(on_false, bdd_profile, max_count);

    if (!new_node->on_true && on_true) {
      u64 on_true_counter = bdd_profile.counters.at(on_true->get_id());
      hit_rate_t on_true_fraction = normalize_fraction(
          static_cast<hit_rate_t>(on_true_counter) / max_count);
      node_id_t bdd_node_id = on_true->get_id();
      new_node->on_true =
          new ProfilerNode(nullptr, on_true_fraction, bdd_node_id);
    }

    if (!new_node->on_false && on_false) {
      u64 on_false_counter = bdd_profile.counters.at(on_false->get_id());
      hit_rate_t on_false_fraction = normalize_fraction(
          static_cast<hit_rate_t>(on_false_counter) / max_count);
      node_id_t bdd_node_id = on_false->get_id();
      new_node->on_false =
          new ProfilerNode(nullptr, on_false_fraction, bdd_node_id);
    }

    if (new_node->on_true) {
      new_node->on_true->prev = new_node;
    }

    if (new_node->on_false) {
      new_node->on_false->prev = new_node;
    }

    result = new_node;
    return NodeVisitAction::STOP;
  });

  return result;
}

static bdd_profile_t build_random_bdd_profile(const BDD *bdd) {
  bdd_profile_t bdd_profile;

  bdd_profile.meta.total_packets = 100'000;
  bdd_profile.meta.total_bytes = std::max(64, RandomEngine::generate() % 1500);
  bdd_profile.meta.avg_pkt_size =
      bdd_profile.meta.total_packets / bdd_profile.meta.total_bytes;

  const Node *root = bdd->get_root();
  bdd_profile.counters[root->get_id()] = bdd_profile.meta.total_packets;

  root->visit_nodes([&bdd_profile](const Node *node) {
    assert(bdd_profile.counters.find(node->get_id()) !=
           bdd_profile.counters.end());
    u64 current_counter = bdd_profile.counters[node->get_id()];

    switch (node->get_type()) {
    case NodeType::BRANCH: {
      const Branch *branch = static_cast<const Branch *>(node);

      const Node *on_true = branch->get_on_true();
      const Node *on_false = branch->get_on_false();

      assert(on_true);
      assert(on_false);

      u64 on_true_counter = RandomEngine::generate() % (current_counter + 1);
      u64 on_false_counter = current_counter - on_true_counter;

      bdd_profile.counters[on_true->get_id()] = on_true_counter;
      bdd_profile.counters[on_false->get_id()] = on_false_counter;
    } break;
    case NodeType::CALL: {
      const Call *call_node = static_cast<const Call *>(node);
      const call_t &call = call_node->get_call();

      if (call.function_name == "map_get" || call.function_name == "map_put" ||
          call.function_name == "map_erase") {
        bdd_profile_t::map_stats_t map_stats;
        map_stats.node = node->get_id();
        map_stats.total_packets = current_counter;
        map_stats.total_flows = RandomEngine::generate() % current_counter;
        map_stats.avg_pkts_per_flow = current_counter / map_stats.total_flows;
        for (u64 i = 0; i < map_stats.total_flows; i++) {
          map_stats.packets_per_flow.push_back(map_stats.avg_pkts_per_flow);
        }
        bdd_profile.map_stats.push_back(map_stats);
      }

      if (node->get_next()) {
        const Node *next = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }
    } break;
    case NodeType::ROUTE: {
      if (node->get_next()) {
        const Node *next = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }
    } break;
    }

    return NodeVisitAction::VISIT_CHILDREN;
  });

  return bdd_profile;
}

Profiler::Profiler(const BDD *bdd, const bdd_profile_t &_bdd_profile)
    : bdd_profile(_bdd_profile) {
  const Node *bdd_root = bdd->get_root();

  assert(bdd_profile.counters.find(bdd_root->get_id()) !=
         bdd_profile.counters.end());
  u64 max_count = bdd_profile.counters.at(bdd_root->get_id());

  root = build_profiler_tree(bdd_root, bdd_profile, max_count);

  for (const bdd_profile_t::map_stats_t &map_stats : bdd_profile.map_stats) {
    const Node *node = bdd->get_node_by_id(map_stats.node);
    assert(node && "Node not found");

    constraints_t constraints = node->get_ordered_branch_constraints();
    ProfilerNode *profiler_node = get_node(constraints);
    assert(profiler_node && "Profiler node not found");

    assert(node->get_type() == NodeType::CALL);
    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    assert(call.function_name == "map_get" || call.function_name == "map_put" ||
           call.function_name == "map_erase");

    assert(call.args.find("key") != call.args.end());
    klee::ref<klee::Expr> flow_id = call.args.at("key").in;

    FlowStats flow_stats = {
        .flow_id = flow_id,
        .total_packets = map_stats.total_packets,
        .total_flows = map_stats.total_flows,
        .avg_pkts_per_flow = map_stats.avg_pkts_per_flow,
        .packets_per_flow = map_stats.packets_per_flow,
    };

    profiler_node->flows_stats.push_back(flow_stats);
  }
}

Profiler::Profiler(const BDD *bdd)
    : Profiler(bdd, build_random_bdd_profile(bdd)) {}

Profiler::Profiler(const BDD *bdd, const std::string &bdd_profile_fname)
    : Profiler(bdd, parse_bdd_profile(bdd_profile_fname)) {}

Profiler::Profiler(const Profiler &other)
    : bdd_profile(other.bdd_profile),
      root(other.root ? other.root->clone(true) : nullptr) {}

Profiler::Profiler(Profiler &&other)
    : bdd_profile(std::move(bdd_profile)), root(std::move(other.root)) {
  other.root = nullptr;
}

Profiler::~Profiler() {
  if (root) {
    delete root;
  }
}

int Profiler::get_avg_pkt_bytes() const {
  return bdd_profile.meta.avg_pkt_size;
}

ProfilerNode *Profiler::get_node(const constraints_t &constraints) const {
  ProfilerNode *current = root;

  for (klee::ref<klee::Expr> constraint : constraints) {
    if (!current) {
      return nullptr;
    }

    klee::ref<klee::Expr> not_constraint =
        solver_toolbox.exprBuilder->Not(constraint);

    assert(!current->constraint.isNull());

    bool on_true =
        solver_toolbox.are_exprs_always_equal(constraint, current->constraint);
    bool on_false = solver_toolbox.are_exprs_always_equal(not_constraint,
                                                          current->constraint);

    assert(on_true || on_false);

    if (on_true) {
      current = current->on_true;
    } else {
      current = current->on_false;
    }
  }

  return current;
}

static void recursive_update_fractions(ProfilerNode *node,
                                       hit_rate_t parent_old_fraction,
                                       hit_rate_t parent_new_fraction) {
  if (!node) {
    return;
  }

  assert(parent_old_fraction >= 0.0);
  assert(parent_old_fraction <= 1.0);

  assert(parent_new_fraction >= 0.0);
  assert(parent_new_fraction <= 1.0);

  hit_rate_t old_fraction = normalize_fraction(node->fraction);
  hit_rate_t new_fraction = normalize_fraction(
      parent_old_fraction != 0
          ? (parent_new_fraction / parent_old_fraction) * node->fraction
          : 0);

  node->fraction = new_fraction;

  recursive_update_fractions(node->on_true, old_fraction, new_fraction);
  recursive_update_fractions(node->on_false, old_fraction, new_fraction);
}

void Profiler::replace_root(klee::ref<klee::Expr> constraint,
                            hit_rate_t fraction) {
  ProfilerNode *new_node = new ProfilerNode(constraint, 1.0);

  ProfilerNode *node = root;
  root = new_node;

  new_node->on_true = node;
  new_node->on_false = node->clone(false);

  new_node->on_true->prev = new_node;
  new_node->on_false->prev = new_node;

  hit_rate_t fraction_on_true = normalize_fraction(fraction);
  hit_rate_t fraction_on_false =
      normalize_fraction(new_node->fraction - fraction);

  assert(fraction_on_true <= 1.0);
  assert(fraction_on_false <= 1.0);

  assert(fraction_on_true <= new_node->fraction);
  assert(fraction_on_false <= new_node->fraction);

  recursive_update_fractions(new_node->on_true, new_node->fraction,
                             fraction_on_true);
  recursive_update_fractions(new_node->on_false, new_node->fraction,
                             fraction_on_false);
}

void Profiler::append(ProfilerNode *node, klee::ref<klee::Expr> constraint,
                      hit_rate_t fraction) {
  assert(node);

  ProfilerNode *parent = node->prev;

  if (!parent) {
    replace_root(constraint, fraction);
    return;
  }

  ProfilerNode *new_node = new ProfilerNode(constraint, node->fraction);

  assert((parent->on_true == node) || (parent->on_false == node));
  if (parent->on_true == node) {
    parent->on_true = new_node;
  } else {
    parent->on_false = new_node;
  }

  new_node->prev = parent;

  new_node->on_true = node;
  new_node->on_false = node->clone(false);

  hit_rate_t fraction_on_true = normalize_fraction(fraction);
  hit_rate_t fraction_on_false =
      normalize_fraction(new_node->fraction - fraction);

  recursive_update_fractions(new_node->on_true, new_node->fraction,
                             fraction_on_true);
  recursive_update_fractions(new_node->on_false, new_node->fraction,
                             fraction_on_false);
}

void Profiler::remove(ProfilerNode *node) {
  assert(node);

  ProfilerNode *parent = node->prev;
  assert(parent && "Cannot remove the root node");

  // By removing the current node, the parent is no longer needed (its purpose
  // was to differentiate between the on_true and on_false nodes, but now only
  // one side is left).

  ProfilerNode *grandparent = parent->prev;
  ProfilerNode *sibling;

  if (parent->on_true == node) {
    sibling = parent->on_false;
    parent->on_false = nullptr;
  } else {
    sibling = parent->on_true;
    parent->on_true = nullptr;
  }

  hit_rate_t parent_fraction = parent->fraction;

  if (!grandparent) {
    // The parent is the root node.
    assert(root == parent);
    root = sibling;
  } else {
    if (grandparent->on_true == parent) {
      grandparent->on_true = sibling;
    } else {
      grandparent->on_false = sibling;
    }

    sibling->prev = grandparent;
  }

  delete parent;

  hit_rate_t old_fraction = sibling->fraction;
  hit_rate_t new_fraction = parent_fraction;

  sibling->fraction = new_fraction;

  recursive_update_fractions(sibling->on_true, old_fraction, new_fraction);
  recursive_update_fractions(sibling->on_false, old_fraction, new_fraction);
}

void Profiler::remove(const constraints_t &constraints) {
  ProfilerNode *node = get_node(constraints);
  remove(node);
}

void Profiler::insert(const constraints_t &constraints,
                      klee::ref<klee::Expr> constraint, hit_rate_t fraction) {
  ProfilerNode *node = get_node(constraints);
  append(node, constraint, fraction);
}

void Profiler::insert_relative(const constraints_t &constraints,
                               klee::ref<klee::Expr> constraint,
                               hit_rate_t rel_fraction_on_true) {
  ProfilerNode *node = get_node(constraints);
  assert(node);

  hit_rate_t fraction = rel_fraction_on_true * node->fraction;
  append(node, constraint, fraction);
}

void Profiler::scale(const constraints_t &constraints, hit_rate_t factor) {
  ProfilerNode *node = get_node(constraints);
  assert(node);

  hit_rate_t old_fraction = normalize_fraction(node->fraction);
  hit_rate_t new_fraction = normalize_fraction(node->fraction * factor);

  node->fraction = new_fraction;

  recursive_update_fractions(node->on_true, old_fraction, new_fraction);
  recursive_update_fractions(node->on_false, old_fraction, new_fraction);
}

std::optional<hit_rate_t>
Profiler::get_fraction(const constraints_t &constraints) const {
  ProfilerNode *node = get_node(constraints);

  if (!node) {
    return std::nullopt;
  }

  return node->fraction;
}

void Profiler::log_debug() const {
  Log::dbg() << "\n============== Hit Rate Tree ==============\n";
  if (root) {
    root->log_debug();
  }
  Log::dbg() << "===========================================\n\n";
}

std::optional<FlowStats>
Profiler::get_flow_stats(const constraints_t &constraints,
                         klee::ref<klee::Expr> flow_id) const {
  ProfilerNode *node = get_node(constraints);

  FlowStats flow_stats;
  if (node && node->get_flow_stats(flow_id, flow_stats)) {
    return flow_stats;
  }

  return std::nullopt;
}