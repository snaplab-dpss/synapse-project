#include "context.h"
#include "target.h"
#include "tofino/tofino_context.h"
#include "tofino_cpu/tofino_cpu_context.h"
#include "x86/x86_context.h"
#include "module_generator.h"

static pps_t estimate_tput_pps_from_ctx(const Context &ctx) {
  pps_t estimation_pps = 0;

  const std::unordered_map<TargetType, hit_rate_t> &traffic_fractions =
      ctx.get_traffic_fractions();

  for (const auto &[target, traffic_fraction] : traffic_fractions) {
    const TargetContext *target_ctx = nullptr;

    switch (target) {
    case TargetType::Tofino: {
      target_ctx = ctx.get_target_ctx<tofino::TofinoContext>();
    } break;
    case TargetType::TofinoCPU: {
      target_ctx = ctx.get_target_ctx<tofino_cpu::TofinoCPUContext>();
    } break;
    case TargetType::x86: {
      target_ctx = ctx.get_target_ctx<x86::x86Context>();
    } break;
    }

    pps_t target_estimation_pps = target_ctx->estimate_tput_pps();
    estimation_pps += target_estimation_pps * traffic_fraction;
  }

  return estimation_pps;
}

void Context::update_tput_estimate() {
  tput_estimate_pps = estimate_tput_pps_from_ctx(*this);
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

void Context::print_speculations(
    const EP *ep, const std::vector<spec_impl_t> &speculations) const {
  Log::log() << "Speculating EP " << ep->get_id() << ":\n";

  for (const spec_impl_t &speculation : speculations) {
    const BDD *bdd = ep->get_bdd();
    const Node *node = bdd->get_node_by_id(speculation.decision.node);
    pps_t tput = estimate_tput_pps_from_ctx(speculation.ctx);

    Log::log() << "  ";
    Log::log() << tput2str(tput, "pps");
    Log::log() << ":";
    for (auto tf : speculation.ctx.get_traffic_fractions())
      Log::log() << " [" << tf.first << ":" << std::setfill('0') << std::fixed
                 << std::setprecision(5) << tf.second << "]";
    Log::log() << " <- ";
    Log::log() << speculation.decision.module;
    Log::log() << " ";
    if (!speculation.skip.empty()) {
      Log::log() << "skip={";
      for (node_id_t skip : speculation.skip)
        Log::log() << skip << ",";
      Log::log() << "} ";
    }
    Log::log() << "(" << node->dump(true, true) << ")";
    Log::log() << "\n";
  }
}

spec_impl_t Context::peek_speculation_for_future_nodes(
    const spec_impl_t &base_speculation, const EP *ep, const Node *anchor,
    nodes_t future_nodes, TargetType current_target) const {
  const targets_t &targets = ep->get_targets();

  future_nodes.erase(anchor->get_id());

  if (future_nodes.empty()) {
    return base_speculation;
  }

  spec_impl_t speculation = base_speculation;

  std::vector<spec_impl_t> speculations;

  anchor->visit_nodes([this, &speculation, &speculations, current_target, ep,
                       &future_nodes](const Node *node) {
    if (future_nodes.empty()) {
      return NodeVisitAction::STOP;
    }

    auto found_it = future_nodes.find(node->get_id());
    if (found_it == future_nodes.end()) {
      return NodeVisitAction::VISIT_CHILDREN;
    }

    future_nodes.erase(node->get_id());

    spec_impl_t node_speculation = get_best_speculation(
        ep, node, current_target, speculation.ctx, speculation.skip);
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
bool Context::is_better_speculation(const spec_impl_t &old_speculation,
                                    const spec_impl_t &new_speculation,
                                    const EP *ep, const Node *node,
                                    TargetType current_target) const {
  nodes_t old_future_nodes =
      filter_away_nodes(new_speculation.skip, old_speculation.skip);
  nodes_t new_future_nodes =
      filter_away_nodes(old_speculation.skip, new_speculation.skip);

  spec_impl_t peek_old = peek_speculation_for_future_nodes(
      old_speculation, ep, node, old_future_nodes, current_target);

  spec_impl_t peek_new = peek_speculation_for_future_nodes(
      new_speculation, ep, node, new_future_nodes, current_target);

  pps_t old_pps = estimate_tput_pps_from_ctx(peek_old.ctx);
  pps_t new_pps = estimate_tput_pps_from_ctx(peek_new.ctx);

  return new_pps > old_pps;
}

spec_impl_t Context::get_best_speculation(const EP *ep, const Node *node,
                                          TargetType current_target,
                                          Context ctx,
                                          const nodes_t &skip) const {
  std::optional<spec_impl_t> best;

  const targets_t &targets = ep->get_targets();

  for (const Target *target : targets) {
    if (target->type != current_target) {
      continue;
    }

    for (const ModuleGenerator *modgen : target->module_generators) {
      std::optional<spec_impl_t> spec = modgen->speculate(ep, node, ctx);

      if (!spec.has_value()) {
        continue;
      }

      spec->skip.insert(skip.begin(), skip.end());

      if (!best.has_value()) {
        best = spec;
        continue;
      }

      bool is_better =
          is_better_speculation(*best, *spec, ep, node, current_target);

      if (is_better) {
        best = spec;
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

void Context::update_tput_speculation(const EP *ep) {
  const std::vector<EPLeaf> &leaves = ep->get_leaves();

  Context ctx(*this);
  nodes_t skip;

  std::vector<spec_impl_t> speculations;

  for (const EPLeaf &leaf : leaves) {
    const Node *node = leaf.next;

    if (!node) {
      continue;
    }

    TargetType current_target;
    constraints_t constraints;

    if (leaf.node) {
      const Module *module = leaf.node->get_module();
      current_target = module->get_next_target();
      constraints = get_node_constraints(leaf.node);
    } else {
      current_target = ep->get_current_platform();
    }

    node->visit_nodes([this, &speculations, current_target, ep, &ctx,
                       &skip](const Node *node) {
      if (skip.find(node->get_id()) != skip.end()) {
        return NodeVisitAction::VISIT_CHILDREN;
      }

      spec_impl_t speculation =
          get_best_speculation(ep, node, current_target, ctx, skip);
      speculations.push_back(speculation);

      ctx = speculation.ctx;
      skip = speculation.skip;

      if (speculation.next_target.has_value() &&
          speculation.next_target.value() != current_target) {
        return NodeVisitAction::SKIP_CHILDREN;
      }

      return NodeVisitAction::VISIT_CHILDREN;
    });
  }

  // if (ep->get_id() == 61) {
  //   print_speculations(ep, speculations);
  //   // BDDVisualizer::visualize(ep->get_bdd(), true);
  //   DEBUG_PAUSE
  // }

  // EPVisualizer::visualize(ep, false);
  // print_speculations(ep, speculations);
  // DEBUG_PAUSE

  tput_speculation_pps = estimate_tput_pps_from_ctx(ctx);
}

void Context::update_tput_estimates(const EP *ep) {
  update_tput_estimate();
  update_tput_speculation(ep);
}

pps_t Context::get_tput_estimate_pps() const { return tput_estimate_pps; }

pps_t Context::get_tput_speculation_pps() const { return tput_speculation_pps; }