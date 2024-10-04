#include "execution_plan.h"
#include "../targets/targets.h"

static pps_t pps_from_ingress_tput(pps_t ingress, const Profiler &profiler) {
  pps_t egress = 0;

  std::vector<std::pair<const ProfilerNode *, pps_t>> nrp = {
      {profiler.get_root(), ingress}};

  while (!nrp.empty()) {
    auto [node, node_ingress] = nrp.back();
    nrp.pop_back();

    pps_t node_egress = 0;

    for (auto [ep_node_id, tput_calc] : node->annotations) {
      node_egress = tput_calc(node_ingress);
      node_ingress = node_egress;
    }

    if (node->on_true == nullptr && node->on_false == nullptr) {
      egress += node_egress;
      continue;
    }

    if (node->on_true != nullptr && node->on_false != nullptr) {
      ProfilerNode *on_true = node->on_true;
      ProfilerNode *on_false = node->on_false;

      hit_rate_t total_hr = node->fraction;
      hit_rate_t on_true_rl_hr = on_true->fraction / total_hr;
      hit_rate_t on_false_rl_hr = on_false->fraction / total_hr;

      pps_t in_on_true = on_true_rl_hr * node_egress;
      pps_t in_on_false = on_false_rl_hr * node_egress;

      nrp.push_back({on_true, in_on_true});
      nrp.push_back({on_false, in_on_false});

      continue;
    }

    assert(node->on_true != nullptr);
    assert(node->on_false == nullptr);

    nrp.push_back({node->on_true, node_egress});
  }

  return egress;
}

pps_t unstable_pps_from_ctx(const Context &ctx) {
  const tofino::TofinoContext *tofino_ctx =
      ctx.get_target_ctx<tofino::TofinoContext>();

  const tofino::TNA &tna = tofino_ctx->get_tna();
  const tofino::PerfOracle perf_oracle = tna.get_perf_oracle();

  const std::unordered_map<TargetType, hit_rate_t> &traffic_fractions =
      ctx.get_traffic_fractions();

  const Profiler &profiler = ctx.get_profiler();

  pps_t ingress = perf_oracle.get_max_input_pps();
  pps_t egress = pps_from_ingress_tput(ingress, profiler);

  return egress;
}

pps_t stable_pps_from_ctx(const Context &ctx) {
  const tofino::TofinoContext *tofino_ctx =
      ctx.get_target_ctx<tofino::TofinoContext>();

  const tofino::TNA &tna = tofino_ctx->get_tna();
  const tofino::PerfOracle perf_oracle = tna.get_perf_oracle();

  const std::unordered_map<TargetType, hit_rate_t> &traffic_fractions =
      ctx.get_traffic_fractions();

  const Profiler &profiler = ctx.get_profiler();

  pps_t ingress = perf_oracle.get_max_input_pps();
  pps_t egress = 0;

  pps_t smallest_unstable = ingress;
  pps_t prev_ingress = ingress;
  pps_t diff = smallest_unstable;

  int it = 0;

  while (diff > 10) {
    prev_ingress = ingress;
    egress = pps_from_ingress_tput(ingress, profiler);

    if (egress < ingress) {
      smallest_unstable = ingress;
      ingress = (ingress + egress) / 2;
    } else {
      ingress = (ingress + smallest_unstable) / 2;
    }

    diff = ingress > prev_ingress ? ingress - prev_ingress
                                  : prev_ingress - ingress;
    it++;
  }

  std::cerr << "Iterations: " << it << std::endl;

  return egress;
}

static pps_t pps_from_ctx(const Context &ctx) {
  // pps_t estimation_pps = unstable_pps_from_ctx(ctx);
  pps_t estimation_pps = stable_pps_from_ctx(ctx);
  return estimation_pps;
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
  Log::log() << "Speculating EP " << id << ":\n";

  for (const spec_impl_t &speculation : speculations) {
    const Node *node = bdd->get_node_by_id(speculation.decision.node);
    pps_t tput = pps_from_ctx(speculation.ctx);

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

spec_impl_t
EP::peek_speculation_for_future_nodes(const spec_impl_t &base_speculation,
                                      const Node *anchor, nodes_t future_nodes,
                                      TargetType current_target) const {
  const targets_t &targets = get_targets();

  future_nodes.erase(anchor->get_id());

  if (future_nodes.empty()) {
    return base_speculation;
  }

  spec_impl_t speculation = base_speculation;

  std::vector<spec_impl_t> speculations;

  anchor->visit_nodes([this, &speculation, &speculations, current_target,
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
        node, current_target, speculation.ctx, speculation.skip);
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
                               const Node *node,
                               TargetType current_target) const {
  nodes_t old_future_nodes =
      filter_away_nodes(new_speculation.skip, old_speculation.skip);
  nodes_t new_future_nodes =
      filter_away_nodes(old_speculation.skip, new_speculation.skip);

  spec_impl_t peek_old = peek_speculation_for_future_nodes(
      old_speculation, node, old_future_nodes, current_target);

  spec_impl_t peek_new = peek_speculation_for_future_nodes(
      new_speculation, node, new_future_nodes, current_target);

  pps_t old_pps = pps_from_ctx(peek_old.ctx);
  pps_t new_pps = pps_from_ctx(peek_new.ctx);

  return new_pps > old_pps;
}

spec_impl_t EP::get_best_speculation(const Node *node,
                                     TargetType current_target,
                                     const Context &ctx,
                                     const nodes_t &skip) const {
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
          is_better_speculation(*best, *spec, node, current_target);

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

pps_t EP::speculate_tput_pps() const {
  Context speculative_ctx(ctx);
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
      constraints = ctx.get_node_constraints(leaf.node);
    } else {
      current_target = get_current_platform();
    }

    node->visit_nodes([this, &speculations, current_target, &speculative_ctx,
                       &skip](const Node *node) {
      if (skip.find(node->get_id()) != skip.end()) {
        return NodeVisitAction::VISIT_CHILDREN;
      }

      spec_impl_t speculation =
          get_best_speculation(node, current_target, speculative_ctx, skip);
      speculations.push_back(speculation);

      speculative_ctx = speculation.ctx;
      skip = speculation.skip;

      if (speculation.next_target.has_value() &&
          speculation.next_target.value() != current_target) {
        return NodeVisitAction::SKIP_CHILDREN;
      }

      return NodeVisitAction::VISIT_CHILDREN;
    });
  }

  // if (id == 2) {
  //   print_speculations(speculations);
  //   // BDDVisualizer::visualize(bdd, true);
  //   // EPVisualizer::visualize(this, false);
  //   DEBUG_PAUSE
  // }

  return pps_from_ctx(speculative_ctx);
}

pps_t EP::estimate_tput_pps() const { return pps_from_ctx(ctx); }