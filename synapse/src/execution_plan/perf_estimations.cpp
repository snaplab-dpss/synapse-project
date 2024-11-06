#include "execution_plan.h"
#include "../targets/targets.h"
#include "../visualizers/profiler_visualizer.h"

struct leaf_t {
  const EPNode *node;
  const Node *next;
};

static pps_t egress_tput_from_ctx(const Context &ctx, pps_t ingress) {
  return ctx.get_perf_oracle().estimate_tput(ingress);
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

void EP::print_speculations(
    const std::vector<spec_impl_t> &speculations) const {
  std::stringstream ss;

  ss << "Speculating EP " << id << ":\n";

  for (const spec_impl_t &speculation : speculations) {
    const Node *node = bdd->get_node_by_id(speculation.decision.node);

    ss << "  ";
    ss << spec2str(speculation, bdd.get());
    ss << "\n";
  }

  ss << "\n";

  Log::dbg() << ss.str();
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

  pps_t old_pps = egress_tput_from_ctx(peek_old.ctx, ingress);
  pps_t new_pps = egress_tput_from_ctx(peek_new.ctx, ingress);

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
    assert(leaf.next);

    leaf.next->visit_nodes(
        [this, &speculations, &spec_ctx, &skip, ingress](const Node *node,
                                                         cookie_t *cookie) {
          spec_cookie_t *spec_cookie = static_cast<spec_cookie_t *>(cookie);

          if (skip.find(node->get_id()) != skip.end()) {
            return NodeVisitAction::VISIT_CHILDREN;
          }

          spec_impl_t speculation = get_best_speculation(
              node, spec_cookie->target, spec_ctx, skip, ingress);
          speculations.push_back(speculation);

          spec_ctx = speculation.ctx;
          skip = speculation.skip;

          if (speculation.next_target.has_value()) {
            spec_cookie->target = *speculation.next_target;
          }

          return NodeVisitAction::VISIT_CHILDREN;
        },
        std::make_unique<spec_cookie_t>(
            leaf.node ? leaf.node->get_module()->get_next_target()
                      : initial_target));
  }

  auto egress_from_ingress = [spec_ctx](pps_t ingress) {
    return egress_tput_from_ctx(spec_ctx, ingress);
  };

  pps_t egress = find_stable_tput(ingress, egress_from_ingress);

  // if (id == 27 || id == 100) {
  //   print_speculations(speculations);
  //   spec_ctx.debug();
  //   // BDDViz::visualize(bdd.get(), false);
  //   // EPViz::visualize(this, false);
  //   // ProfilerViz::visualize(bdd.get(), spec_ctx.get_profiler(), false);
  //   debug_active_leaves();
  //   DEBUG_PAUSE
  // }

  cached_tput_speculation = egress;
  return egress;
}

pps_t EP::estimate_tput_pps() const {
  if (cached_tput_estimation.has_value()) {
    return *cached_tput_estimation;
  }

  pps_t max_ingress = ctx.get_perf_oracle().get_max_input_pps();

  auto egress_from_ingress = [this](pps_t ingress) {
    return egress_tput_from_ctx(ctx, ingress);
  };

  pps_t egress = find_stable_tput(max_ingress, egress_from_ingress);
  cached_tput_estimation = egress;

  return egress;
}