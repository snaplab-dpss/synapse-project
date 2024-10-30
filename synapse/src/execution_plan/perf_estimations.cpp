#include "execution_plan.h"
#include "../targets/targets.h"
#include "../visualizers/profiler_visualizer.h"

struct leaf_tput_t {
  const EPNode *node;
  const Node *next;
  pps_t tput;
};

struct leaf_t {
  const EPNode *node;
  const Node *next;
};

static std::vector<leaf_tput_t> compute_leaves_egress_tput(const EP *ep,
                                                           pps_t ingress) {
  std::vector<leaf_tput_t> leaves_tput;

  const EPNode *root = ep->get_root();
  const std::vector<EPLeaf> &active_leaves = ep->get_active_leaves();

  if (!root) {
    assert(active_leaves.size() == 1);

    leaves_tput.push_back({
        active_leaves[0].node,
        active_leaves[0].next,
        ingress,
    });

    return leaves_tput;
  }

  const Context &ctx = ep->get_ctx();
  const Profiler &profiler = ctx.get_profiler();

  struct node_tput_t {
    const EPNode *node;
    pps_t ingress;
  };

  std::vector<node_tput_t> nodes({{root, ingress}});

  while (!nodes.empty()) {
    node_tput_t node_tput = nodes.back();
    nodes.pop_back();

    const EPNode *node = node_tput.node;
    const Module *module = node->get_module();

    pps_t node_ingress = node_tput.ingress;
    pps_t node_egress = module->compute_egress_tput(ep, node_ingress);

    auto leaf_it =
        std::find_if(active_leaves.begin(), active_leaves.end(),
                     [node](const EPLeaf &leaf) { return leaf.node == node; });

    if (leaf_it != active_leaves.end()) {
      leaves_tput.push_back({leaf_it->node, leaf_it->next, node_egress});
      continue;
    }

    const std::vector<EPNode *> &children = node->get_children();

    if (children.empty()) {
      leaves_tput.push_back({node, nullptr, node_egress});
      continue;
    }

    if (children.size() == 1) {
      nodes.push_back({children[0], node_egress});
      continue;
    }

    assert(children.size() == 2 && "Node with more than 2 children.");

    const EPNode *lhs = children[0];
    const EPNode *rhs = children[1];

    hit_rate_t lhs_hr = profiler.get_hr(ep, lhs);
    hit_rate_t rhs_hr = profiler.get_hr(ep, rhs);

    hit_rate_t total_hr = lhs_hr + rhs_hr;
    hit_rate_t lhs_rl_hr = lhs_hr / total_hr;
    hit_rate_t rhs_rl_hr = rhs_hr / total_hr;

    pps_t lhs_ingress = lhs_rl_hr * node_egress;
    pps_t rhs_ingress = rhs_rl_hr * node_egress;

    nodes.push_back({lhs, lhs_ingress});
    nodes.push_back({rhs, rhs_ingress});
  }

  return leaves_tput;
}

pps_t EP::pps_from_ingress_tput(pps_t ingress) const {
  std::vector<leaf_tput_t> leaves_tput =
      compute_leaves_egress_tput(this, ingress);

  pps_t egress = 0;
  for (const leaf_tput_t &leaf_tput : leaves_tput) {
    egress += leaf_tput.tput;
  }

  return egress;
}

struct speculative_data_t : public cookie_t {
  constraints_t constraints;
  std::unordered_map<node_id_t, klee::ref<klee::Expr>> pending_constraints;

  speculative_data_t(const constraints_t &_constraints)
      : constraints(_constraints) {}

  speculative_data_t(const speculative_data_t &other)
      : constraints(other.constraints),
        pending_constraints(other.pending_constraints) {}

  virtual speculative_data_t *clone() const {
    return new speculative_data_t(*this);
  }
};

void EP::print_speculations(
    const std::vector<spec_impl_t> &speculations) const {
  std::stringstream ss;

  ss << "Speculating EP " << id << ":\n";

  for (const spec_impl_t &speculation : speculations) {
    const Node *node = bdd->get_node_by_id(speculation.decision.node);

    ss << "  ";
    for (auto tf : speculation.ctx.get_traffic_fractions())
      ss << " [" << tf.first << ":" << std::setfill('0') << std::fixed
         << std::setprecision(5) << tf.second << "]";
    ss << " <- ";
    ss << speculation.decision.module;
    ss << " ";
    if (!speculation.skip.empty()) {
      ss << "skip={";
      for (node_id_t skip : speculation.skip)
        ss << skip << ",";
      ss << "} ";
    }
    ss << "(" << node->dump(true, true) << ")";
    ss << "\n";
  }

  Log::dbg() << ss.str();
  Log::dbg() << "\n";
}

spec_impl_t EP::peek_speculation_for_future_nodes(
    const spec_impl_t &base_speculation, const Node *anchor,
    nodes_t future_nodes, TargetType current_target, pps_t ingress) const {
  const targets_t &targets = get_targets();

  future_nodes.erase(anchor->get_id());

  if (future_nodes.empty()) {
    return base_speculation;
  }

  spec_impl_t speculation = base_speculation;

  std::vector<spec_impl_t> speculations;

  anchor->visit_nodes([this, &speculation, &speculations, current_target,
                       ingress, &future_nodes](const Node *node) {
    if (future_nodes.empty()) {
      return NodeVisitAction::STOP;
    }

    auto found_it = future_nodes.find(node->get_id());
    if (found_it == future_nodes.end()) {
      return NodeVisitAction::VISIT_CHILDREN;
    }

    future_nodes.erase(node->get_id());

    spec_impl_t node_speculation = get_best_speculation(
        node, current_target, speculation.ctx, speculation.skip, ingress);
    speculations.push_back(node_speculation);

    speculation = node_speculation;

    if (speculation.next_target.has_value() &&
        speculation.next_target != current_target) {
      return NodeVisitAction::SKIP_CHILDREN;
    }

    return NodeVisitAction::VISIT_CHILDREN;
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

  pps_t old_pps = peek_old.perf_estimator(peek_old.ctx, node, ingress);
  pps_t new_pps = peek_new.perf_estimator(peek_new.ctx, node, ingress);

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

      assert(best.has_value());
      assert(spec.has_value());

      bool is_better =
          is_better_speculation(*best, *spec, node, current_target, ingress);

      if (is_better) {
        best = *spec;
      }
    }
  }

  if (!best.has_value()) {
    PANIC("No module to speculative execute\n"
          "Target: %s\n"
          "Node:   %s\n",
          to_string(current_target).c_str(), node->dump(true).c_str());
  }

  return *best;
}

static std::vector<leaf_t> get_leaves(const EP *ep) {
  const EPNode *root = ep->get_root();
  const std::vector<EPLeaf> &active_leaves = ep->get_active_leaves();

  if (!root) {
    assert(active_leaves.size() == 1);
    return {{active_leaves[0].node, active_leaves[0].next}};
  }

  std::vector<leaf_t> leaves;

  root->visit_nodes([&leaves, &active_leaves](const EPNode *node) {
    auto leaf_it =
        std::find_if(active_leaves.begin(), active_leaves.end(),
                     [node](const EPLeaf &leaf) { return leaf.node == node; });

    if (leaf_it != active_leaves.end()) {
      leaves.emplace_back(node, leaf_it->next);
    } else if (node->get_children().empty()) {
      leaves.emplace_back(node, nullptr);
    }

    return EPNodeVisitAction::VISIT_CHILDREN;
  });

  return leaves;
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
  pps_t ingress = estimate_tput_pps();
  pps_t egress = 0;

  struct spec_leaf_t {
    const Node *next;
    TargetType target;
    pps_t tput;
  };

  std::vector<spec_leaf_t> spec_leaves;
  std::vector<leaf_t> leaves = get_leaves(this);

  for (const leaf_t &leaf : leaves) {
    pps_t tput = ingress;

    if (leaf.node) {
      tput = ctx.get_profiler().get_hr(this, leaf.node) * ingress;
    }

    if (!leaf.next) {
      egress += tput;
    } else {
      spec_leaves.push_back({
          leaf.next,
          leaf.node ? leaf.node->get_module()->get_next_target()
                    : initial_target,
          tput,
      });
    }
  }

  std::vector<spec_impl_t> speculations;
  Context speculative_ctx = ctx;
  nodes_t skip;

  while (!spec_leaves.empty()) {
    spec_leaf_t leaf = spec_leaves.back();
    spec_leaves.pop_back();

    hit_rate_t total_hr = ctx.get_profiler().get_hr(leaf.next);

    if (skip.find(leaf.next->get_id()) != skip.end()) {
      std::vector<const Node *> children = leaf.next->get_children();
      for (const Node *child : children) {
        hit_rate_t hr = ctx.get_profiler().get_hr(child);
        pps_t child_tput = (hr / total_hr) * leaf.tput;
        spec_leaves.push_back({child, leaf.target, child_tput});
      }
      continue;
    }

    spec_impl_t speculation = get_best_speculation(
        leaf.next, leaf.target, speculative_ctx, skip, leaf.tput);
    speculations.push_back(speculation);

    speculative_ctx = speculation.ctx;
    skip = speculation.skip;

    pps_t tput =
        speculation.perf_estimator(speculation.ctx, leaf.next, leaf.tput);

    std::vector<const Node *> children = leaf.next->get_children();

    if (children.empty()) {
      egress += tput;
      continue;
    }

    for (const Node *child : children) {
      hit_rate_t hr = ctx.get_profiler().get_hr(child);
      pps_t child_tput = (hr / total_hr) * tput;
      spec_leaves.push_back({child, leaf.target, child_tput});
    }
  }

  // if (id == 2) {
  //   print_speculations(speculations);
  //   speculative_ctx.debug();
  //   // BDDVisualizer::visualize(bdd, false);
  //   EPVisualizer::visualize(this, false);
  //   DEBUG_PAUSE
  // }

  return egress;
}

pps_t EP::estimate_tput_pps() const {
  const tofino::TofinoContext *tofino_ctx =
      ctx.get_target_ctx<tofino::TofinoContext>();

  pps_t ingress = tofino_ctx->get_tna().get_perf_oracle().get_max_input_pps();
  pps_t egress = 0;

  pps_t smallest_unstable = ingress;
  pps_t prev_ingress = ingress;
  pps_t diff = smallest_unstable;

  // Algorithm for converging to a stable throughput (basically a binary
  // search). This hopefully doesn't take many iterations...
  while (diff > STABLE_TPUT_PRECISION) {
    prev_ingress = ingress;
    egress = pps_from_ingress_tput(ingress);

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