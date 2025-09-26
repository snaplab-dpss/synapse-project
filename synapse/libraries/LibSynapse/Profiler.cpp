#include <LibSynapse/Profiler.h>
#include <LibSynapse/EPNode.h>
#include <LibCore/Debug.h>
#include <LibCore/RandomEngine.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>
#include <LibCore/Net.h>

#include <iomanip>
#include <list>

namespace LibSynapse {

using LibBDD::bdd_node_id_t;
using LibBDD::BDDNodeType;
using LibBDD::BDDNodeVisitAction;
using LibBDD::Branch;
using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;
using LibBDD::Route;
using LibBDD::RouteOp;

using LibCore::expr_addr_to_obj_addr;
using LibCore::expr_to_string;
using LibCore::pretty_print_expr;
using LibCore::solver_toolbox;
using LibCore::symbol_t;

namespace {

ProfilerNode *build_profiler_tree(const BDD *bdd, const BDDNode *node, const bdd_profile_t *bdd_profile, u64 max_count,
                                  const std::unordered_set<u16> available_devs) {
  ProfilerNode *prof_node = nullptr;

  while (node) {
    switch (node->get_type()) {
    case BDDNodeType::Call: {
      if (!node->get_next()) {
        const u64 counter   = bdd_profile->counters.at(node->get_id());
        const hit_rate_t hr = hit_rate_t(counter, max_count);
        prof_node           = new ProfilerNode(nullptr, hr, node->get_id());
      }
      node = node->get_next();
    } break;
    case BDDNodeType::Branch: {
      const Branch *branch            = dynamic_cast<const Branch *>(node);
      const BDDNode *on_true          = branch->get_on_true();
      const BDDNode *on_false         = branch->get_on_false();
      klee::ref<klee::Expr> condition = branch->get_condition();

      const u64 counter   = bdd_profile->counters.at(node->get_id());
      const hit_rate_t hr = hit_rate_t(counter, max_count);
      prof_node           = new ProfilerNode(condition, hr, node->get_id());

      if (on_true) {
        prof_node->on_true       = build_profiler_tree(bdd, on_true, bdd_profile, max_count, available_devs);
        prof_node->on_true->prev = prof_node;
      }

      if (on_false) {
        prof_node->on_false       = build_profiler_tree(bdd, on_false, bdd_profile, max_count, available_devs);
        prof_node->on_false->prev = prof_node;
      }

      node = nullptr;
    } break;
    case BDDNodeType::Route: {
      const Route *route      = dynamic_cast<const Route *>(node);
      const RouteOp operation = route->get_operation();

      assert(bdd_profile->forwarding_stats.find(node->get_id()) != bdd_profile->forwarding_stats.end());

      const u64 counter = bdd_profile->counters.at(node->get_id());
      const hit_rate_t hr(counter, max_count);

      prof_node = new ProfilerNode(nullptr, hr, node->get_id());

      const bdd_profile_t::fwd_stats_t &fwd_stats = bdd_profile->forwarding_stats.at(node->get_id());

      prof_node->forwarding_stats.emplace();
      prof_node->forwarding_stats->operation = operation;
      prof_node->forwarding_stats->drop      = hit_rate_t(fwd_stats.drop, max_count);
      prof_node->forwarding_stats->flood     = hit_rate_t(fwd_stats.flood, max_count);

      u16 most_used_fwd_port       = 0;
      u64 most_used_fwd_port_count = 0;
      for (const auto &[device, dev_counter] : fwd_stats.ports) {
        prof_node->forwarding_stats->ports[device] = hit_rate_t(dev_counter, max_count);
        if (dev_counter > most_used_fwd_port_count) {
          most_used_fwd_port       = device;
          most_used_fwd_port_count = dev_counter;
        }
      }

      prof_node->original_forwarding_stats = prof_node->forwarding_stats;

      // Hack: sometimes the forwarding stats hit rate calculations don't exactly match the node's hr (due to imprecisions on the calculations).
      // Let's fix that by slightly adjusting the hr of the forwarding stats.

      const hit_rate_t fwd_total_hr = prof_node->forwarding_stats->calculate_total_hr();
      const hit_rate_t delta        = hr - fwd_total_hr;

      if (fwd_stats.get_total_fwd() > fwd_stats.drop) {
        prof_node->forwarding_stats->ports[most_used_fwd_port] = prof_node->forwarding_stats->ports[most_used_fwd_port] + delta;
      } else {
        prof_node->forwarding_stats->drop = prof_node->forwarding_stats->drop + delta;
      }

      assert(prof_node->forwarding_stats->calculate_total_hr() == hr && "ProfilerNode forwarding stats do not match the node's hr");

      for (const u16 dev : bdd->get_candidate_fwd_devs(route)) {
        if (available_devs.contains(dev)) {
          prof_node->candidate_fwd_ports.insert(dev);
        }
      }

      node = node->get_next();
    } break;
    }
  }

  return prof_node;
}

} // namespace

ProfilerNode::ProfilerNode(klee::ref<klee::Expr> _constraint, hit_rate_t _fraction)
    : constraint(_constraint), fraction(_fraction), on_true(nullptr), on_false(nullptr), prev(nullptr) {}

ProfilerNode::ProfilerNode(klee::ref<klee::Expr> _constraint, hit_rate_t _fraction, bdd_node_id_t _node_id)
    : constraint(_constraint), fraction(_fraction), bdd_node_id(_node_id), on_true(nullptr), on_false(nullptr), prev(nullptr) {}

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

  new_node->forwarding_stats          = forwarding_stats;
  new_node->original_forwarding_stats = original_forwarding_stats;
  new_node->candidate_fwd_ports       = candidate_fwd_ports;

  if (on_true) {
    new_node->on_true       = on_true->clone(keep_bdd_info);
    new_node->on_true->prev = new_node;
  }

  if (on_false) {
    new_node->on_false       = on_false->clone(keep_bdd_info);
    new_node->on_false->prev = new_node;
  }

  return new_node;
}

std::ostream &operator<<(std::ostream &os, const ProfilerNode &node) {
  os << "<";
  os << "hr=" << node.fraction;

  if (!node.constraint.isNull()) {
    os << ", ";
    os << "cond=" << pretty_print_expr(node.constraint, true);
  }

  if (node.bdd_node_id) {
    os << ", ";
    os << "bdd_node=" << *node.bdd_node_id;
  }

  if (node.forwarding_stats) {
    os << ", ";
    os << "fwd_stats={";
    os << "drop=" << node.forwarding_stats->drop << ", ";
    os << "flood=" << node.forwarding_stats->flood << ", ";
    os << "ports={";
    for (const auto &[port, hr] : node.forwarding_stats->ports) {
      if (hr == 0) {
        continue;
      }
      os << port << "=" << hr << ", ";
    }
    os << "}";
    os << "}";
  }

  os << ">";

  return os;
}

void ProfilerNode::debug(int lvl) const {
  std::cerr << *this;
  std::cerr << "\n";

  if (on_true) {
    lvl++;

    // std::cerr << std::string(2 * lvl, '|');
    for (int i = 0; i < 2 * lvl; i++)
      std::cerr << ((i % 2 != 0) ? "|" : " ");

    std::cerr << "[T] ";
    on_true->debug(lvl);
    lvl--;
  }

  if (on_false) {
    lvl++;

    for (int i = 0; i < 2 * lvl; i++)
      std::cerr << ((i % 2 != 0) ? "|" : " ");
    // std::cerr << std::string(2 * lvl, '|');

    std::cerr << "[F] ";
    on_false->debug(lvl);
    lvl--;
  }
}

flow_stats_t Profiler::get_flow_stats(const BDDNode *node, klee::ref<klee::Expr> flow_id) const {
  auto found_it = flows_stats_per_bdd_node.find(node->get_id());
  if (found_it == flows_stats_per_bdd_node.end()) {
    panic("Flow stats not found for BDD node %s", node->dump(true).c_str());
  }

  for (const flow_stats_t &stats : found_it->second) {
    if (solver_toolbox.are_exprs_always_equal(flow_id, stats.flow_id)) {
      return stats;
    }
  }

  panic("Flow stats not found for BDD node %s (flow not found)", node->dump(true).c_str());
}

fwd_stats_t Profiler::get_fwd_stats(const BDDNode *node) const {
  ProfilerNode *profiler_node = get_node(node);
  assert(profiler_node && "Profiler node not found");
  assert(profiler_node->forwarding_stats.has_value());
  return profiler_node->forwarding_stats.value();
}

std::unordered_set<u16> Profiler::get_candidate_fwd_ports(const BDDNode *node) const {
  ProfilerNode *profiler_node = get_node(node);
  assert(profiler_node && "Profiler node not found");
  return profiler_node->candidate_fwd_ports;
}

Profiler::Profiler(const BDD *bdd, const bdd_profile_t &_bdd_profile, const std::unordered_set<u16> &available_devs)
    : bdd_profile(new bdd_profile_t(_bdd_profile)), avg_pkt_size(bdd_profile->meta.bytes / bdd_profile->meta.pkts), root(nullptr), cache() {
  const BDDNode *bdd_root = bdd->get_root();

  assert(bdd_profile->counters.find(bdd_root->get_id()) != bdd_profile->counters.end() && "Root node not found");
  const u64 max_count = bdd_profile->counters.at(bdd_root->get_id());

  root = std::shared_ptr<ProfilerNode>(build_profiler_tree(bdd, bdd_root, bdd_profile.get(), max_count, available_devs));

  for (const auto &[map_addr, map_stats] : bdd_profile->stats_per_map) {
    for (const auto &node_map_stats : map_stats.nodes) {
      const BDDNode *node = bdd->get_node_by_id(node_map_stats.node);
      assert(node && "BDDNode not found");
      assert(node->get_type() == BDDNodeType::Call && "Invalid node type");
      const Call *call_node = dynamic_cast<const Call *>(node);
      const call_t &call    = call_node->get_call();

      assert((call.function_name == "map_get" || call.function_name == "map_put" || call.function_name == "map_erase") && "Invalid call");

      assert(call.args.find("key") != call.args.end() && "Key not found");
      klee::ref<klee::Expr> flow_id = call.args.at("key").in;

      const flow_stats_t flow_stats = {
          .flow_id       = flow_id,
          .pkts          = node_map_stats.pkts,
          .flows         = node_map_stats.flows,
          .pkts_per_flow = node_map_stats.pkts_per_flow,
      };

      flows_stats_per_bdd_node[node->get_id()].push_back(flow_stats);
    }
  }
}

Profiler::Profiler(const Profiler &other)
    : bdd_profile(other.bdd_profile), avg_pkt_size(other.avg_pkt_size), root(other.root), flows_stats_per_bdd_node(other.flows_stats_per_bdd_node),
      cache(other.cache) {}

Profiler::Profiler(Profiler &&other)
    : bdd_profile(std::move(other.bdd_profile)), avg_pkt_size(other.avg_pkt_size), root(std::move(other.root)),
      flows_stats_per_bdd_node(std::move(other.flows_stats_per_bdd_node)), cache(std::move(other.cache)) {
  other.root.reset();
}

Profiler &Profiler::operator=(const Profiler &other) {
  if (this == &other) {
    return *this;
  }

  assert(bdd_profile == other.bdd_profile);
  assert(avg_pkt_size == other.avg_pkt_size);

  root                     = other.root;
  flows_stats_per_bdd_node = other.flows_stats_per_bdd_node;
  cache                    = other.cache;

  return *this;
}

const bdd_profile_t *Profiler::get_bdd_profile() const { return bdd_profile.get(); }

bytes_t Profiler::get_avg_pkt_bytes() const { return avg_pkt_size; }

ProfilerNode *Profiler::get_node(const std::vector<klee::ref<klee::Expr>> &constraints) const {
  ProfilerNode *current = root.get();

  std::unordered_set<size_t> index_of_used_constraints;
  while (index_of_used_constraints.size() != constraints.size()) {
    if (!current) {
      return nullptr;
    }

    assert(!current->constraint.isNull() && "Invalid profiler node");

    bool found = false;
    for (size_t i = 0; i < constraints.size(); ++i) {
      if (index_of_used_constraints.contains(i)) {
        continue;
      }

      klee::ref<klee::Expr> cnstr = constraints[i];

      klee::ConstraintManager manager;
      manager.addConstraint(current->constraint);

      const bool always_true  = solver_toolbox.is_expr_always_true(manager, cnstr);
      const bool always_false = solver_toolbox.is_expr_always_false(manager, cnstr);

      if (!always_true && !always_false) {
        continue;
      }

      if (always_true) {
        current = current->on_true;
      } else {
        current = current->on_false;
      }

      found = true;
      index_of_used_constraints.insert(i);
    }

    if (!found) {
      std::cerr << "\n";
      std::cerr << "Constraints:\n";
      for (const klee::ref<klee::Expr> &c : constraints) {
        std::cerr << "  " << pretty_print_expr(c, true) << "\n";
      }
      std::cerr << "Current constraints:\n";
      std::cerr << "  " << pretty_print_expr(current->constraint, true) << "\n";
      panic("Could find profiler node (invalid constraints)");
    }
  }

  assert(current && "Profiler node not found");
  return current;
}

ProfilerNode *Profiler::get_node(const BDDNode *node) const {
  auto found_it = cache.n2p.find(node->get_id());
  if (found_it != cache.n2p.end()) {
    assert(found_it->second && "Invalid profiler node");
    return found_it->second;
  }

  const std::vector<klee::ref<klee::Expr>> constraints = node->get_ordered_branch_constraints();
  ProfilerNode *profiler_node                          = get_node(constraints);

  if (!profiler_node) {
    panic("Profiler node not found");
  }

  cache.n2p[node->get_id()] = profiler_node;
  cache.p2n[profiler_node]  = node->get_id();

  return profiler_node;
}

ProfilerNode *Profiler::get_node(const EPNode *node) const {
  auto found_it = cache.e2p.find(node->get_id());
  if (found_it != cache.e2p.end()) {
    assert(found_it->second && "Invalid profiler node");
    return found_it->second;
  }

  std::vector<klee::ref<klee::Expr>> constraints = node->get_constraints();
  ProfilerNode *profiler_node                    = get_node(constraints);

  if (!profiler_node) {
    panic("Profiler node not found");
  }

  cache.e2p[node->get_id()] = profiler_node;
  cache.p2e[profiler_node]  = node->get_id();

  return profiler_node;
}

void Profiler::replace_root(klee::ref<klee::Expr> constraint, hit_rate_t fraction) {
  panic("Attempted to replace Profiler root node");

  ProfilerNode *new_node = new ProfilerNode(constraint, 1_hr);

  ProfilerNode *node = root.get();
  root               = std::shared_ptr<ProfilerNode>(new_node);

  new_node->on_true  = node;
  new_node->on_false = node->clone(false);

  new_node->on_true->prev  = new_node;
  new_node->on_false->prev = new_node;

  const hit_rate_t fraction_on_true  = fraction;
  const hit_rate_t fraction_on_false = new_node->fraction - fraction;

  assert(fraction_on_true <= new_node->fraction && "Invalid fraction");
  assert(fraction_on_false <= new_node->fraction && "Invalid fraction");

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

  assert(((parent->on_true == node) || (parent->on_false == node)) && "Invalid node");
  if (parent->on_true == node) {
    parent->on_true = new_node;
  } else {
    parent->on_false = new_node;
  }

  new_node->prev = parent;

  new_node->on_true       = node;
  new_node->on_true->prev = new_node;

  new_node->on_false       = node->clone(false);
  new_node->on_false->prev = new_node;

  const hit_rate_t fraction_on_true  = fraction;
  const hit_rate_t fraction_on_false = new_node->fraction - fraction;

  recursive_update_fractions(new_node->on_true, new_node->fraction, fraction_on_true);
  recursive_update_fractions(new_node->on_false, new_node->fraction, fraction_on_false);
}

ProfilerNode::family_t ProfilerNode::get_family() {
  family_t family = {
      .node        = this,
      .parent      = prev,
      .grandparent = nullptr,
      .sibling     = nullptr,
  };

  if (family.parent) {
    family.grandparent = family.parent->prev;
    family.sibling     = (family.parent->on_true == this) ? family.parent->on_false : family.parent->on_true;
  }

  return family;
}

void Profiler::remove(ProfilerNode *node) {
  ProfilerNode::family_t family = node->get_family();

  assert(family.parent && "Cannot remove the root node");
  const hit_rate_t parent_fraction = family.parent->fraction;

  // By removing the current node, the parent is no longer needed (its purpose
  // was to differentiate between the on_true and on_false nodes, but now only
  // one side is left).

  assert(family.grandparent && "Cannot remove the root node");

  if (family.grandparent->on_true == family.parent) {
    family.grandparent->on_true = family.sibling;
  } else {
    family.grandparent->on_false = family.sibling;
  }

  family.sibling->prev = family.grandparent;

  if (family.parent->on_true == family.sibling) {
    family.parent->on_true = nullptr;
  } else {
    family.parent->on_false = nullptr;
  }

  delete family.parent;

  const hit_rate_t old_fraction = family.sibling->fraction;
  const hit_rate_t new_fraction = parent_fraction;

  update_fractions(family.sibling, new_fraction);

  recursive_update_fractions(family.sibling->on_true, old_fraction, new_fraction);
  recursive_update_fractions(family.sibling->on_false, old_fraction, new_fraction);
}

void Profiler::remove_until(ProfilerNode *target, ProfilerNode *stopping_node) {
  assert(target != nullptr);
  assert(stopping_node != nullptr);

  ProfilerNode *parent = target->prev;
  assert(parent);

  while (stopping_node->prev && stopping_node->prev != parent) {
    ProfilerNode *sibling = stopping_node->get_family().sibling;
    assert(sibling);
    remove(sibling);
  }
}

void Profiler::remove(const std::vector<klee::ref<klee::Expr>> &constraints) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);
  remove(node);
}

void Profiler::remove_until(const std::vector<klee::ref<klee::Expr>> &target, const std::vector<klee::ref<klee::Expr>> &stopping_constraints) {
  clone_tree_if_shared();
  ProfilerNode *target_node   = get_node(target);
  ProfilerNode *stopping_node = get_node(stopping_constraints);
  remove_until(target_node, stopping_node);
}

void Profiler::insert_relative(const std::vector<klee::ref<klee::Expr>> &constraints, klee::ref<klee::Expr> constraint,
                               hit_rate_t rel_fraction_on_true) {
  clone_tree_if_shared();
  ProfilerNode *node        = get_node(constraints);
  const hit_rate_t fraction = hit_rate_t{rel_fraction_on_true * node->fraction};
  append(node, constraint, fraction);
}

void Profiler::scale(const std::vector<klee::ref<klee::Expr>> &constraints, double factor) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);

  const hit_rate_t old_fraction = node->fraction;
  const hit_rate_t new_fraction = node->fraction * factor;

  update_fractions(node, new_fraction);

  recursive_update_fractions(node->on_true, old_fraction, new_fraction);
  recursive_update_fractions(node->on_false, old_fraction, new_fraction);
}

bool Profiler::can_set(const std::vector<klee::ref<klee::Expr>> &constraints, hit_rate_t new_hr) const {
  return can_set(get_node(constraints), new_hr);
}

void Profiler::set(const std::vector<klee::ref<klee::Expr>> &constraints, hit_rate_t new_hr) {
  clone_tree_if_shared();
  set(get_node(constraints), new_hr);
}

void Profiler::set_relative(const std::vector<klee::ref<klee::Expr>> &constraints, hit_rate_t parent_relative_hr) {
  clone_tree_if_shared();

  ProfilerNode *node = get_node(constraints);
  assert(node);

  const ProfilerNode::family_t family = node->get_family();
  assert(family.parent);

  const hit_rate_t new_fraction = hit_rate_t(family.parent->fraction * parent_relative_hr);

  set(node, new_fraction);
}

hit_rate_t Profiler::get_hr(const BDDNode *node) const {
  ProfilerNode *profiler_node = get_node(node);
  assert(profiler_node);
  return profiler_node->fraction;
}

hit_rate_t Profiler::get_hr(const EPNode *node) const {
  ProfilerNode *profiler_node = get_node(node);
  assert(profiler_node);
  return profiler_node->fraction;
}

hit_rate_t Profiler::get_hr(const std::vector<klee::ref<klee::Expr>> &constraints) const {
  ProfilerNode *profiler_node = get_node(constraints);
  assert(profiler_node);
  return profiler_node->fraction;
}

void Profiler::clear_cache() const {
  cache.n2p.clear();
  cache.p2n.clear();
  cache.e2p.clear();
  cache.p2e.clear();
}

void Profiler::debug() const {
  std::cerr << "\n============== Hit Rate Tree ==============\n";
  if (root) {
    root->debug();
  }
  std::cerr << "===========================================\n\n";
}

void Profiler::clone_tree_if_shared() {
  clear_cache();

  assert(root && "Invalid profiler root node");

  // No need to clone a tree that is not shared.
  if (root.use_count() == 1) {
    return;
  }

  root = std::shared_ptr<ProfilerNode>(root->clone(true));
}

void Profiler::translate(SymbolManager *symbol_manager, const BDDNode *reordered_node, const std::vector<symbol_translation_t> &translated_symbols) {
  clone_tree_if_shared();

  std::unordered_map<std::string, std::string> translations;
  for (const auto &[old_symbol, new_symbol] : translated_symbols) {
    translations[old_symbol.name] = new_symbol.name;
  }

  std::vector<ProfilerNode *> nodes{get_node(reordered_node)};

  while (!nodes.empty()) {
    ProfilerNode *node = nodes.back();
    nodes.pop_back();

    node->constraint = symbol_manager->translate(node->constraint, translations);

    if (node->on_true) {
      nodes.push_back(node->on_true);
    }

    if (node->on_false) {
      nodes.push_back(node->on_false);
    }
  }
}

void Profiler::replace_constraint(ProfilerNode *node, klee::ref<klee::Expr> constraint) {
  clone_tree_if_shared();
  node->constraint = constraint;
}

void Profiler::replace_constraint(const std::vector<klee::ref<klee::Expr>> &constraints, klee::ref<klee::Expr> constraint) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);
  replace_constraint(node, constraint);
}

rw_fractions_t Profiler::get_cond_map_put_rw_profile_fractions(const Call *map_get) const {
  const call_t &mg_call = map_get->get_call();
  assert(mg_call.function_name == "map_get" && "Unexpected function");

  klee::ref<klee::Expr> obj = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;

  symbol_t map_has_this_key = map_get->get_local_symbol("map_has_this_key");

  branch_direction_t success_check = map_get->get_map_get_success_check();
  assert(success_check.branch && "Map get success check not found");

  const BDDNode *read          = success_check.direction ? success_check.branch->get_on_true() : success_check.branch->get_on_false();
  const BDDNode *write_attempt = success_check.direction ? success_check.branch->get_on_false() : success_check.branch->get_on_true();

  std::vector<const Call *> future_map_puts = write_attempt->get_future_functions({"map_put"});
  assert(future_map_puts.size() >= 1 && "map_put not found");

  const BDDNode *write = nullptr;
  for (const Call *map_put : future_map_puts) {
    const call_t &mp_call = map_put->get_call();
    assert(mp_call.function_name == "map_put" && "Unexpected function");

    klee::ref<klee::Expr> o = mp_call.args.at("map").expr;
    klee::ref<klee::Expr> k = mp_call.args.at("key").in;

    if (solver_toolbox.are_exprs_always_equal(o, obj) && solver_toolbox.are_exprs_always_equal(k, key)) {
      write = map_put;
      break;
    }
  }

  assert(write && "map_put not found");

  rw_fractions_t fractions{
      .read          = get_hr(read),
      .write_attempt = get_hr(write_attempt),
      .write         = get_hr(write),
  };

  return fractions;
}

bool Profiler::can_update_fractions(ProfilerNode *node) const {
  if (!node) {
    return false;
  }

  // Not a forwarding node, so we can directly update the fraction.
  if (!node->forwarding_stats.has_value()) {
    return true;
  }

  assert(node->original_forwarding_stats.has_value());
  const fwd_stats_t &original_fwd_stats = node->original_forwarding_stats.value();
  if (original_fwd_stats.calculate_total_hr() != 0_hr) {
    return true;
  }

  const hit_rate_t old_fraction = node->fraction;
  if (old_fraction != 0_hr) {
    return true;
  }

  fwd_stats_t &fwd_stats = node->forwarding_stats.value();
  if (fwd_stats.operation == RouteOp::Forward && original_fwd_stats.ports.empty()) {
    return false;
  }

  return true;
}

void Profiler::update_fractions(ProfilerNode *node, hit_rate_t new_fraction) {
  if (!node) {
    return;
  }

  const hit_rate_t old_fraction = node->fraction;
  node->fraction                = new_fraction;

  if (!node->forwarding_stats.has_value()) {
    return;
  }

  assert(node->original_forwarding_stats.has_value());

  fwd_stats_t &fwd_stats                = node->forwarding_stats.value();
  const fwd_stats_t &original_fwd_stats = node->original_forwarding_stats.value();

  if (original_fwd_stats.calculate_total_hr() != 0_hr) {
    const hit_rate_t total_hr = original_fwd_stats.calculate_total_hr();
    const double dhr          = new_fraction / total_hr;

    fwd_stats = original_fwd_stats.scale(dhr);
    assert(new_fraction == fwd_stats.calculate_total_hr());

    return;
  }

  if (old_fraction != 0_hr) {
    assert(old_fraction == fwd_stats.calculate_total_hr());

    const hit_rate_t total_fraction = fwd_stats.calculate_total_hr();
    const double dhr                = new_fraction / total_fraction;
    fwd_stats                       = original_fwd_stats.scale(dhr);

    assert(new_fraction == fwd_stats.calculate_total_hr());
    return;
  }

  switch (fwd_stats.operation) {
  case RouteOp::Drop: {
    fwd_stats.drop = new_fraction;
  } break;
  case RouteOp::Broadcast: {
    fwd_stats.flood = new_fraction;
  } break;
  case RouteOp::Forward: {
    if (new_fraction == 0_hr && !original_fwd_stats.ports.empty()) {
      for (auto &[port, hr] : original_fwd_stats.ports) {
        fwd_stats.ports[port] = 0_hr;
      }
    } else {
      assert_or_panic(!node->candidate_fwd_ports.empty(), "Forwarding operation but no valid candidate ports");
      const hit_rate_t hr_per_port = new_fraction / node->candidate_fwd_ports.size();
      for (const u16 port : node->candidate_fwd_ports) {
        fwd_stats.ports[port] = hr_per_port;
      }
    }

  } break;
  }
}

bool Profiler::can_recursive_update_fractions(ProfilerNode *node, hit_rate_t parent_old_fraction) const {
  if (!node) {
    return false;
  }

  const hit_rate_t old_fraction = node->fraction;

  if (parent_old_fraction == 0_hr) {
    const ProfilerNode::family_t family = node->get_family();
    if (family.sibling) {
      // There is not enough information to make profiling forwarding decisions.
      return false;
    }
  }

  if (!can_update_fractions(node)) {
    return false;
  }

  if (node->on_true && !can_recursive_update_fractions(node->on_true, old_fraction)) {
    return false;
  }

  if (node->on_false && !can_recursive_update_fractions(node->on_false, old_fraction)) {
    return false;
  }

  return true;
}

void Profiler::recursive_update_fractions(ProfilerNode *node, hit_rate_t parent_old_fraction, hit_rate_t parent_new_fraction) {
  if (!node) {
    return;
  }

  if (parent_old_fraction == parent_new_fraction) {
    return;
  }

  const hit_rate_t old_fraction = node->fraction;

  hit_rate_t new_fraction;
  if (parent_old_fraction == 0_hr) {
    const ProfilerNode::family_t family = node->get_family();

    if (family.sibling) {
      // There is not enough information to make profiling forwarding decisions.
      panic("Not enough profiling information: distribution of traffic across siblings is unknown");
    } else {
      new_fraction = parent_new_fraction;
    }
  } else {
    new_fraction = node->fraction * (parent_new_fraction / parent_old_fraction);
  }

  update_fractions(node, new_fraction);

  recursive_update_fractions(node->on_true, old_fraction, new_fraction);
  recursive_update_fractions(node->on_false, old_fraction, new_fraction);

  if (node->on_true && node->on_false) {
    assert(node->on_true->fraction + node->on_false->fraction == new_fraction);
  }
}

bool Profiler::can_set(ProfilerNode *node, hit_rate_t new_hr) const {
  if (!node) {
    return false;
  }

  const hit_rate_t old_fraction = node->fraction;
  const hit_rate_t new_fraction = new_hr;

  if (old_fraction == new_fraction) {
    return true;
  }

  if ((node->on_true && !can_recursive_update_fractions(node->on_true, old_fraction)) ||
      (node->on_false && !can_recursive_update_fractions(node->on_false, old_fraction))) {
    return false;
  }

  const ProfilerNode::family_t family = node->get_family();
  if (family.sibling) {
    const hit_rate_t sibling_old_fraction = family.sibling->fraction;
    if ((family.sibling->on_true && !can_recursive_update_fractions(family.sibling->on_true, sibling_old_fraction)) ||
        (family.sibling->on_false && !can_recursive_update_fractions(family.sibling->on_false, sibling_old_fraction))) {
      return false;
    }
  }

  return true;
}

void Profiler::set(ProfilerNode *node, hit_rate_t new_hr) {
  const hit_rate_t old_fraction = node->fraction;
  const hit_rate_t new_fraction = new_hr;

  if (old_fraction == new_fraction) {
    return;
  }

  const ProfilerNode::family_t closest_family = node->get_family();

  bool changed_parent_hr        = false;
  ProfilerNode::family_t family = closest_family;
  while (family.parent && family.parent->fraction < new_fraction) {
    family.parent->fraction = new_fraction;
    family                  = family.parent->get_family();
    changed_parent_hr       = true;
  }

  if (changed_parent_hr) {
    set(family.sibling, family.parent->fraction - new_fraction);
  }

  update_fractions(node, new_fraction);

  recursive_update_fractions(node->on_true, old_fraction, new_fraction);
  recursive_update_fractions(node->on_false, old_fraction, new_fraction);

  if (closest_family.sibling) {
    const hit_rate_t sibling_old_fraction = closest_family.sibling->fraction;
    const hit_rate_t sibling_new_fraction = closest_family.parent->fraction - new_fraction;

    update_fractions(closest_family.sibling, sibling_new_fraction);

    recursive_update_fractions(closest_family.sibling->on_true, sibling_old_fraction, sibling_new_fraction);
    recursive_update_fractions(closest_family.sibling->on_false, sibling_old_fraction, sibling_new_fraction);
  }
}

} // namespace LibSynapse