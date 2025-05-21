#include <LibSynapse/Profiler.h>
#include <LibSynapse/EPNode.h>
#include <LibCore/Debug.h>
#include <LibCore/RandomEngine.h>
#include <LibCore/Expr.h>
#include <LibCore/Solver.h>
#include <LibCore/Net.h>

#include <iomanip>

namespace LibSynapse {

namespace {

ProfilerNode *build_profiler_tree(const LibBDD::Node *node, const LibBDD::bdd_profile_t *bdd_profile, u64 max_count) {
  ProfilerNode *prof_node = nullptr;

  while (node) {
    switch (node->get_type()) {
    case LibBDD::NodeType::Call: {
      if (!node->get_next()) {
        const u64 counter   = bdd_profile->counters.at(node->get_id());
        const hit_rate_t hr = hit_rate_t(counter, max_count);
        prof_node           = new ProfilerNode(nullptr, hr, node->get_id());
      }
      node = node->get_next();
    } break;
    case LibBDD::NodeType::Branch: {
      const LibBDD::Branch *branch    = dynamic_cast<const LibBDD::Branch *>(node);
      const LibBDD::Node *on_true     = branch->get_on_true();
      const LibBDD::Node *on_false    = branch->get_on_false();
      klee::ref<klee::Expr> condition = branch->get_condition();

      const u64 counter   = bdd_profile->counters.at(node->get_id());
      const hit_rate_t hr = hit_rate_t(counter, max_count);
      prof_node           = new ProfilerNode(condition, hr, node->get_id());

      if (on_true) {
        prof_node->on_true = build_profiler_tree(on_true, bdd_profile, max_count);
      }

      if (on_false) {
        prof_node->on_false = build_profiler_tree(on_false, bdd_profile, max_count);
      }

      node = nullptr;
    } break;
    case LibBDD::NodeType::Route: {
      const LibBDD::Route *route      = dynamic_cast<const LibBDD::Route *>(node);
      const LibBDD::RouteOp operation = route->get_operation();

      assert(bdd_profile->forwarding_stats.find(node->get_id()) != bdd_profile->forwarding_stats.end());

      const u64 counter = bdd_profile->counters.at(node->get_id());
      const hit_rate_t hr(counter, max_count);
      prof_node = new ProfilerNode(nullptr, hr, node->get_id());

      prof_node->forwarding_stats.emplace();
      prof_node->forwarding_stats->operation = operation;
      prof_node->forwarding_stats->drop      = hit_rate_t(bdd_profile->forwarding_stats.at(node->get_id()).drop, max_count);
      prof_node->forwarding_stats->flood     = hit_rate_t(bdd_profile->forwarding_stats.at(node->get_id()).flood, max_count);
      for (const auto &[device, dev_counter] : bdd_profile->forwarding_stats.at(node->get_id()).ports) {
        prof_node->forwarding_stats->ports[device] = hit_rate_t(dev_counter, max_count);
      }

      prof_node->original_forwarding_stats = prof_node->forwarding_stats;

      // Hack: sometimes the forwarding stats hit rate calculations don't exactly match the node's hr (due to imprecisions on the calculations).
      // Let's fix that by slightly adjusting the hr of the forwarding stats.

      const hit_rate_t fwd_total_hr = prof_node->forwarding_stats->calculate_total_hr();
      const hit_rate_t delta        = hr - fwd_total_hr;

      if (fwd_total_hr > prof_node->forwarding_stats->drop) {
        const u16 most_used_fwd_port                           = prof_node->forwarding_stats->get_most_used_fwd_port();
        prof_node->forwarding_stats->ports[most_used_fwd_port] = prof_node->forwarding_stats->ports[most_used_fwd_port] + delta;
      } else {
        prof_node->forwarding_stats->drop = prof_node->forwarding_stats->drop + delta;
      }

      assert(prof_node->forwarding_stats->calculate_total_hr() == hr && "ProfilerNode forwarding stats do not match the node's hr");

      node = node->get_next();
    } break;
    }
  }

  return prof_node;
}

LibBDD::bdd_profile_t build_random_bdd_profile(const LibBDD::BDD *bdd) {
  LibBDD::bdd_profile_t bdd_profile;

  bdd_profile.meta.pkts  = 100'000;
  bdd_profile.meta.bytes = bdd_profile.meta.pkts * std::max(MIN_PKT_SIZE_BYTES, LibCore::SingletonRandomEngine::generate() % MAX_PKT_SIZE_BYTES);

  const LibBDD::Node *root             = bdd->get_root();
  bdd_profile.counters[root->get_id()] = bdd_profile.meta.pkts;
  const std::vector<u16> devices       = bdd->get_devices();

  root->visit_nodes([&bdd_profile, devices](const LibBDD::Node *node) {
    assert(bdd_profile.counters.find(node->get_id()) != bdd_profile.counters.end() && "LibBDD::Node counter not found");
    u64 current_counter = bdd_profile.counters[node->get_id()];

    switch (node->get_type()) {
    case LibBDD::NodeType::Branch: {
      const LibBDD::Branch *branch = dynamic_cast<const LibBDD::Branch *>(node);

      const LibBDD::Node *on_true  = branch->get_on_true();
      const LibBDD::Node *on_false = branch->get_on_false();

      assert(on_true && "Branch node without on_true");
      assert(on_false && "Branch node without on_false");

      u64 on_true_counter  = LibCore::SingletonRandomEngine::generate() % (current_counter + 1);
      u64 on_false_counter = current_counter - on_true_counter;

      bdd_profile.counters[on_true->get_id()]  = on_true_counter;
      bdd_profile.counters[on_false->get_id()] = on_false_counter;
    } break;
    case LibBDD::NodeType::Call: {
      const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
      const LibBDD::call_t &call    = call_node->get_call();

      if (call.function_name == "map_get" || call.function_name == "map_put" || call.function_name == "map_erase") {
        klee::ref<klee::Expr> map_addr = call.args.at("map").expr;

        LibBDD::bdd_profile_t::map_stats_t::node_t map_stats;
        map_stats.node  = node->get_id();
        map_stats.pkts  = current_counter;
        map_stats.flows = std::max(1ul, LibCore::SingletonRandomEngine::generate() % current_counter);

        u64 avg_pkts_per_flow = current_counter / map_stats.flows;
        for (u64 i = 0; i < map_stats.flows; i++) {
          map_stats.pkts_per_flow.push_back(avg_pkts_per_flow);
        }

        bdd_profile.stats_per_map[LibCore::expr_addr_to_obj_addr(map_addr)].nodes.push_back(map_stats);
      }

      if (node->get_next()) {
        const LibBDD::Node *next             = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }
    } break;
    case LibBDD::NodeType::Route: {
      if (node->get_next()) {
        const LibBDD::Node *next             = node->get_next();
        bdd_profile.counters[next->get_id()] = current_counter;
      }

      const LibBDD::Route *route_node = dynamic_cast<const LibBDD::Route *>(node);
      const LibBDD::RouteOp operation = route_node->get_operation();

      switch (operation) {
      case LibBDD::RouteOp::Drop: {
        bdd_profile.forwarding_stats[node->get_id()].drop = current_counter;
      } break;
      case LibBDD::RouteOp::Broadcast: {
        bdd_profile.forwarding_stats[node->get_id()].flood = current_counter;
      } break;
      case LibBDD::RouteOp::Forward: {
        klee::ref<klee::Expr> dst_device = route_node->get_dst_device();
        if (LibCore::is_constant(dst_device)) {
          const u16 device = LibCore::solver_toolbox.value_from_expr(dst_device);
          assert(std::find(devices.begin(), devices.end(), device) != devices.end() && "Invalid device");
          bdd_profile.forwarding_stats[node->get_id()].ports[device] = current_counter;
        } else {
          const u16 device                                           = devices[LibCore::SingletonRandomEngine::generate() % devices.size()];
          bdd_profile.forwarding_stats[node->get_id()].ports[device] = current_counter;
        }
      } break;
      }
    } break;
    }

    return LibBDD::NodeVisitAction::Continue;
  });

  return bdd_profile;
}

void update_fractions(ProfilerNode *node, hit_rate_t new_fraction) {
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

  if (old_fraction != 0_hr) {
    assert(old_fraction == fwd_stats.calculate_total_hr());

    const double dhr = new_fraction / old_fraction;

    fwd_stats.drop  = fwd_stats.drop * dhr;
    fwd_stats.flood = fwd_stats.flood * dhr;
    for (auto &[_, hr] : fwd_stats.ports) {
      hr = hr * dhr;
    }

    assert(new_fraction == fwd_stats.calculate_total_hr());
  } else if (original_fwd_stats.calculate_total_hr() != 0_hr) {
    const hit_rate_t total_hr = original_fwd_stats.calculate_total_hr();

    fwd_stats.drop  = new_fraction * (original_fwd_stats.drop / total_hr);
    fwd_stats.flood = new_fraction * (original_fwd_stats.flood / total_hr);

    for (auto &[port, hr] : original_fwd_stats.ports) {
      fwd_stats.ports[port] = new_fraction * (hr / total_hr);
    }

    assert(new_fraction == fwd_stats.calculate_total_hr());
  } else {
    switch (fwd_stats.operation) {
    case LibBDD::RouteOp::Drop: {
      fwd_stats.drop = new_fraction;
    } break;
    case LibBDD::RouteOp::Broadcast: {
      fwd_stats.flood = new_fraction;
    } break;
    case LibBDD::RouteOp::Forward: {
      if (new_fraction == 0_hr && !original_fwd_stats.ports.empty()) {
        for (auto &[port, hr] : original_fwd_stats.ports) {
          fwd_stats.ports[port] = 0_hr;
        }
      } else {
        // There is not enough information to make profiling forwarding decisions.
        // This is because the original profiler didn't exercise this forwarding path, so we have no idea how to distribute traffic across the ports.
        panic("Not enough information to make profiling forwarding decisions");
      }
    } break;
    }
  }
}

void recursive_update_fractions(ProfilerNode *node, hit_rate_t parent_old_fraction, hit_rate_t parent_new_fraction) {
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
} // namespace

ProfilerNode::ProfilerNode(klee::ref<klee::Expr> _constraint, hit_rate_t _fraction)
    : constraint(_constraint), fraction(_fraction), on_true(nullptr), on_false(nullptr), prev(nullptr) {}

ProfilerNode::ProfilerNode(klee::ref<klee::Expr> _constraint, hit_rate_t _fraction, LibBDD::node_id_t _node_id)
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

  new_node->flows_stats               = flows_stats;
  new_node->forwarding_stats          = forwarding_stats;
  new_node->original_forwarding_stats = original_forwarding_stats;

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
    os << "cond=" << LibCore::pretty_print_expr(node.constraint, true);
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

flow_stats_t ProfilerNode::get_flow_stats(klee::ref<klee::Expr> flow_id) const {
  for (const flow_stats_t &stats : flows_stats) {
    if (LibCore::solver_toolbox.are_exprs_always_equal(flow_id, stats.flow_id)) {
      return stats;
    }
  }

  panic("Flow stats not found");
}

fwd_stats_t Profiler::get_fwd_stats(const std::vector<klee::ref<klee::Expr>> &constraints) const {
  ProfilerNode *profiler_node = get_node(constraints);
  assert(profiler_node && "Profiler node not found");
  assert(profiler_node->forwarding_stats.has_value());
  return profiler_node->forwarding_stats.value();
}

fwd_stats_t Profiler::get_fwd_stats(const EPNode *node) const {
  ProfilerNode *profiler_node = get_node(node);
  assert(profiler_node && "Profiler node not found");
  assert(profiler_node->forwarding_stats.has_value());
  return profiler_node->forwarding_stats.value();
}

fwd_stats_t Profiler::get_fwd_stats(const LibBDD::Node *node) const {
  ProfilerNode *profiler_node = get_node(node);
  assert(profiler_node && "Profiler node not found");
  assert(profiler_node->forwarding_stats.has_value());
  return profiler_node->forwarding_stats.value();
}

Profiler::Profiler(const LibBDD::BDD *bdd, const LibBDD::bdd_profile_t &_bdd_profile)
    : bdd_profile(new LibBDD::bdd_profile_t(_bdd_profile)), root(nullptr), avg_pkt_size(bdd_profile->meta.bytes / bdd_profile->meta.pkts), cache() {
  const LibBDD::Node *bdd_root = bdd->get_root();

  assert(bdd_profile->counters.find(bdd_root->get_id()) != bdd_profile->counters.end() && "Root node not found");
  u64 max_count = bdd_profile->counters.at(bdd_root->get_id());

  root = std::shared_ptr<ProfilerNode>(build_profiler_tree(bdd_root, bdd_profile.get(), max_count));

  for (const auto &[map_addr, map_stats] : bdd_profile->stats_per_map) {
    for (const auto &node_map_stats : map_stats.nodes) {
      const LibBDD::Node *node = bdd->get_node_by_id(node_map_stats.node);
      assert(node && "LibBDD::Node not found");

      std::vector<klee::ref<klee::Expr>> constraints = node->get_ordered_branch_constraints();
      ProfilerNode *profiler_node                    = get_node(constraints);
      assert(profiler_node && "Profiler node not found");

      assert(node->get_type() == LibBDD::NodeType::Call && "Invalid node type");
      const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
      const LibBDD::call_t &call    = call_node->get_call();

      assert((call.function_name == "map_get" || call.function_name == "map_put" || call.function_name == "map_erase") && "Invalid call");

      assert(call.args.find("key") != call.args.end() && "Key not found");
      klee::ref<klee::Expr> flow_id = call.args.at("key").in;

      flow_stats_t flow_stats = {
          .flow_id       = flow_id,
          .pkts          = node_map_stats.pkts,
          .flows         = node_map_stats.flows,
          .pkts_per_flow = node_map_stats.pkts_per_flow,
      };

      profiler_node->flows_stats.push_back(flow_stats);
    }
  }
}

Profiler::Profiler(const LibBDD::BDD *bdd) : Profiler(bdd, build_random_bdd_profile(bdd)) {}

Profiler::Profiler(const LibBDD::BDD *bdd, const std::filesystem::path &bdd_profile_fname)
    : Profiler(bdd, LibBDD::parse_bdd_profile(bdd_profile_fname)) {}

Profiler::Profiler(const Profiler &other) : bdd_profile(other.bdd_profile), root(other.root), avg_pkt_size(other.avg_pkt_size), cache(other.cache) {}

Profiler::Profiler(Profiler &&other)
    : bdd_profile(std::move(other.bdd_profile)), root(std::move(other.root)), avg_pkt_size(other.avg_pkt_size), cache(std::move(other.cache)) {
  other.root = nullptr;
}

Profiler &Profiler::operator=(const Profiler &other) {
  if (this == &other) {
    return *this;
  }

  bdd_profile  = other.bdd_profile;
  root         = other.root;
  avg_pkt_size = other.avg_pkt_size;
  cache        = other.cache;

  return *this;
}

const LibBDD::bdd_profile_t *Profiler::get_bdd_profile() const { return bdd_profile.get(); }

bytes_t Profiler::get_avg_pkt_bytes() const { return avg_pkt_size; }

ProfilerNode *Profiler::get_node(const std::vector<klee::ref<klee::Expr>> &constraints) const {
  ProfilerNode *current = root.get();

  for (klee::ref<klee::Expr> cnstr : constraints) {
    if (!current) {
      return nullptr;
    }

    assert(!current->constraint.isNull() && "Invalid profiler node");

    klee::ConstraintManager manager;
    manager.addConstraint(current->constraint);

    const bool always_true  = LibCore::solver_toolbox.is_expr_always_true(manager, cnstr);
    const bool always_false = LibCore::solver_toolbox.is_expr_always_false(manager, cnstr);

    if (!always_true && !always_false) {
      std::cerr << "\n";
      std::cerr << "Constraints:\n";
      for (const klee::ref<klee::Expr> &c : constraints) {
        std::cerr << "  " << LibCore::pretty_print_expr(c, true) << "\n";
      }
      std::cerr << "Current constraints:\n";
      std::cerr << "  " << LibCore::pretty_print_expr(current->constraint, true) << "\n";
      debug();
      panic("Could find profiler node (invalid constraints).");
    }

    if (always_true) {
      current = current->on_true;
    } else {
      current = current->on_false;
    }
  }

  assert(current && "Profiler node not found");
  return current;
}

ProfilerNode *Profiler::get_node(const LibBDD::Node *node) const {
  auto found_it = cache.n2p.find(node->get_id());
  if (found_it != cache.n2p.end()) {
    assert(found_it->second && "Invalid profiler node");
    return found_it->second;
  }

  std::vector<klee::ref<klee::Expr>> constraints = node->get_ordered_branch_constraints();
  ProfilerNode *profiler_node                    = get_node(constraints);

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

  family.parent->on_true  = nullptr;
  family.parent->on_false = nullptr;
  delete family.parent;

  node->on_true  = nullptr;
  node->on_false = nullptr;
  delete node;

  const hit_rate_t old_fraction = family.sibling->fraction;
  const hit_rate_t new_fraction = parent_fraction;

  update_fractions(family.sibling, new_fraction);

  recursive_update_fractions(family.sibling->on_true, old_fraction, new_fraction);
  recursive_update_fractions(family.sibling->on_false, old_fraction, new_fraction);
}

void Profiler::remove(const std::vector<klee::ref<klee::Expr>> &constraints) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);
  remove(node);
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

void Profiler::set(const std::vector<klee::ref<klee::Expr>> &constraints, hit_rate_t new_hr) {
  clone_tree_if_shared();
  ProfilerNode *node = get_node(constraints);

  const hit_rate_t old_fraction = node->fraction;
  const hit_rate_t new_fraction = new_hr;

  update_fractions(node, new_fraction);

  recursive_update_fractions(node->on_true, old_fraction, new_fraction);
  recursive_update_fractions(node->on_false, old_fraction, new_fraction);
}

hit_rate_t Profiler::get_hr(const LibBDD::Node *node) const {
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

flow_stats_t Profiler::get_flow_stats(const std::vector<klee::ref<klee::Expr>> &constraints, klee::ref<klee::Expr> flow_id) const {
  return get_node(constraints)->get_flow_stats(flow_id);
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

void Profiler::translate(LibCore::SymbolManager *symbol_manager, const LibBDD::Node *reordered_node,
                         const std::vector<LibBDD::translated_symbol_t> &translated_symbols) {
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

rw_fractions_t Profiler::get_cond_map_put_rw_profile_fractions(const LibBDD::Call *map_get) const {
  const LibBDD::call_t &mg_call = map_get->get_call();
  assert(mg_call.function_name == "map_get" && "Unexpected function");

  klee::ref<klee::Expr> obj = mg_call.args.at("map").expr;
  klee::ref<klee::Expr> key = mg_call.args.at("key").in;

  LibCore::symbol_t map_has_this_key = map_get->get_local_symbol("map_has_this_key");

  LibBDD::branch_direction_t success_check = map_get->get_map_get_success_check();
  assert(success_check.branch && "Map get success check not found");

  const LibBDD::Node *read          = success_check.direction ? success_check.branch->get_on_true() : success_check.branch->get_on_false();
  const LibBDD::Node *write_attempt = success_check.direction ? success_check.branch->get_on_false() : success_check.branch->get_on_true();

  std::vector<const LibBDD::Call *> future_map_puts = write_attempt->get_future_functions({"map_put"});
  assert(future_map_puts.size() >= 1 && "map_put not found");

  const LibBDD::Node *write = nullptr;
  for (const LibBDD::Call *map_put : future_map_puts) {
    const LibBDD::call_t &mp_call = map_put->get_call();
    assert(mp_call.function_name == "map_put" && "Unexpected function");

    klee::ref<klee::Expr> o = mp_call.args.at("map").expr;
    klee::ref<klee::Expr> k = mp_call.args.at("key").in;

    if (LibCore::solver_toolbox.are_exprs_always_equal(o, obj) && LibCore::solver_toolbox.are_exprs_always_equal(k, key)) {
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

} // namespace LibSynapse