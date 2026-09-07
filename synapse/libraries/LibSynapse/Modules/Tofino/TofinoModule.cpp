#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/GlobalStats.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
#include <LibSynapse/Modules/Tofino/DataStructures/ComputeAction.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>
#include <klee/util/ExprVisitor.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Route;
using LibBDD::RouteOp;

namespace {
class ActionExprCompatibilityChecker : public klee::ExprVisitor::ExprVisitor {
private:
  std::unordered_set<std::string> symbols;
  int operations;
  bool compatible;
  size_t max_symbols;
  int max_operations;

public:
  ActionExprCompatibilityChecker(size_t _max_symbols = 1, int _max_operations = 1)
      : operations(0), compatible(true), max_symbols(_max_symbols), max_operations(_max_operations) {}

  bool is_compatible() const { return compatible; }

  Action visit_incompatible_op() {
    compatible = false;
    return Action::skipChildren();
  }

  Action visit_compatible_op() {
    operations++;
    if (operations > max_operations) {
      compatible = false;
      return Action::skipChildren();
    }
    return Action::doChildren();
  }

  Action visitRead(const klee::ReadExpr &e) override final {
    const std::string name = e.updates.root->name;
    symbols.insert(name);
    if (symbols.size() > max_symbols) {
      compatible = false;
      return Action::skipChildren();
    }
    return Action::doChildren();
  }

  Action visitConcat(const klee::ConcatExpr &e) override final { return Action::doChildren(); }

  Action visitSelect(const klee::SelectExpr &e) override final { return visit_compatible_op(); }
  Action visitExtract(const klee::ExtractExpr &e) override final { return visit_compatible_op(); }
  Action visitZExt(const klee::ZExtExpr &e) override final { return visit_compatible_op(); }
  Action visitSExt(const klee::SExtExpr &e) override final { return visit_compatible_op(); }
  Action visitAdd(const klee::AddExpr &e) override final { return visit_compatible_op(); }
  Action visitSub(const klee::SubExpr &e) override final { return visit_compatible_op(); }
  Action visitNot(const klee::NotExpr &e) override final { return visit_compatible_op(); }
  Action visitAnd(const klee::AndExpr &e) override final { return visit_compatible_op(); }
  Action visitOr(const klee::OrExpr &e) override final { return visit_compatible_op(); }
  Action visitXor(const klee::XorExpr &e) override final { return visit_compatible_op(); }
  Action visitShl(const klee::ShlExpr &e) override final { return visit_compatible_op(); }
  Action visitLShr(const klee::LShrExpr &e) override final { return visit_compatible_op(); }
  Action visitAShr(const klee::AShrExpr &e) override final { return visit_compatible_op(); }
  Action visitEq(const klee::EqExpr &e) override final { return visit_compatible_op(); }
  Action visitNe(const klee::NeExpr &e) override final { return visit_compatible_op(); }
  Action visitUlt(const klee::UltExpr &e) override final { return visit_compatible_op(); }
  Action visitUle(const klee::UleExpr &e) override final { return visit_compatible_op(); }
  Action visitUgt(const klee::UgtExpr &e) override final { return visit_compatible_op(); }
  Action visitUge(const klee::UgeExpr &e) override final { return visit_compatible_op(); }
  Action visitSlt(const klee::SltExpr &e) override final { return visit_compatible_op(); }
  Action visitSle(const klee::SleExpr &e) override final { return visit_compatible_op(); }
  Action visitSgt(const klee::SgtExpr &e) override final { return visit_compatible_op(); }
  Action visitSge(const klee::SgeExpr &e) override final { return visit_compatible_op(); }

  Action visitMul(const klee::MulExpr &e) override final { return visit_incompatible_op(); }
  Action visitUDiv(const klee::UDivExpr &e) override final { return visit_incompatible_op(); }
  Action visitSDiv(const klee::SDivExpr &e) override final { return visit_incompatible_op(); }
  Action visitURem(const klee::URemExpr &e) override final { return visit_incompatible_op(); }
  Action visitSRem(const klee::SRemExpr &e) override final { return visit_incompatible_op(); }
};

} // namespace

bool TofinoModuleFactory::was_ds_already_used(const EPNode *node, DS_ID ds_id) {
  while (node) {
    if (node->get_module()->get_target() == TargetType::Tofino) {
      const TofinoModule *tofino_module = dynamic_cast<const TofinoModule *>(node->get_module());

      if (tofino_module->get_type() == ModuleType::Tofino_Recirculate) {
        break;
      }

      if (tofino_module->get_generated_ds().contains(ds_id)) {
        // This DS was already generated in an ancestor node targeting Tofino.
        return true;
      }
    }

    node = node->get_prev();
  }

  return false;
}

bool TofinoModuleFactory::was_ds_already_used(const EPNode *leaf, const speculations_t &speculations, const BDDNode *node, addr_t obj, DSImpl ds_impl,
                                              DS_ID ds_id) {
  if (was_ds_already_used(leaf, ds_id)) {
    return true;
  }

  const BDDNode *root = nullptr;
  if (leaf && leaf->get_module()) {
    root = leaf->get_module()->get_node();
  }

  const std::map<std::pair<bdd_node_id_t, addr_t>, DSImpl> &ds_impls_decisions_per_bdd_node_and_obj =
      speculations.ctx.get_ds_impls_decisions_per_bdd_node_and_obj();

  while (node && node != root) {
    auto found_it = ds_impls_decisions_per_bdd_node_and_obj.find({node->get_id(), obj});
    if (found_it != ds_impls_decisions_per_bdd_node_and_obj.end()) {
      if (found_it->second == ds_impl) {
        return true;
      }
    }

    node = node->get_prev();
  }

  return false;
}

TofinoContext *TofinoModuleFactory::get_mutable_tofino_ctx(EP *ep) {
  Context &ctx = ep->get_mutable_ctx();
  return ctx.get_mutable_target_ctx<TofinoContext>();
}

const TofinoContext *TofinoModuleFactory::get_tofino_ctx(const EP *ep) {
  const Context &ctx = ep->get_ctx();
  return ctx.get_target_ctx<TofinoContext>();
}

TNA &TofinoModuleFactory::get_mutable_tna(EP *ep) {
  TofinoContext *ctx = get_mutable_tofino_ctx(ep);
  return ctx->get_mutable_tna();
}

const TNA &TofinoModuleFactory::get_tna(const EP *ep) {
  const TofinoContext *ctx = get_tofino_ctx(ep);
  return ctx->get_tna();
}

bool TofinoModuleFactory::expr_fits_in_action(klee::ref<klee::Expr> expr) {
  ActionExprCompatibilityChecker checker;
  checker.visit(expr);
  return checker.is_compatible();
}

bool TofinoModuleFactory::expr_fits_in_action(klee::ref<klee::Expr> expr, size_t max_symbols) {
  ActionExprCompatibilityChecker checker(max_symbols);
  checker.visit(expr);
  return checker.is_compatible();
}

bool TofinoModuleFactory::expr_is_materializable(klee::ref<klee::Expr> expr) {
  // Can this be computed into a metadata field by an ordinary MAU action? Allows
  // a couple of inputs and a short chain of casts/slices/arithmetic (no mul/div).
  ActionExprCompatibilityChecker checker(/*max_symbols=*/2, /*max_operations=*/8);
  checker.visit(expr);
  return checker.is_compatible();
}

namespace {
class ReadSymbolCollector : public klee::ExprVisitor::ExprVisitor {
public:
  std::unordered_set<std::string> names;
  Action visitRead(const klee::ReadExpr &e) override final {
    names.insert(e.updates.root->name);
    return Action::doChildren();
  }
};

std::unordered_set<std::string> collect_read_symbols(klee::ref<klee::Expr> expr) {
  ReadSymbolCollector collector;
  collector.visit(expr);
  return collector.names;
}
} // namespace

std::optional<klee::ref<klee::Expr>> TofinoModuleFactory::get_register_increment_delta(klee::ref<klee::Expr> write_value,
                                                                                       klee::ref<klee::Expr> read_value) {
  if (write_value->getKind() != klee::Expr::Kind::Add) {
    return {};
  }

  klee::ref<klee::Expr> lhs = write_value->getKid(0);
  klee::ref<klee::Expr> rhs = write_value->getKid(1);

  klee::ref<klee::Expr> delta;
  if (lhs->compare(*read_value) == 0) {
    delta = rhs;
  } else if (rhs->compare(*read_value) == 0) {
    delta = lhs;
  } else {
    return {};
  }

  // The delta must not reference the register's own value.
  const std::unordered_set<std::string> reg_symbols   = collect_read_symbols(read_value);
  const std::unordered_set<std::string> delta_symbols = collect_read_symbols(delta);
  for (const std::string &name : delta_symbols) {
    if (reg_symbols.count(name)) {
      return {};
    }
  }

  return delta;
}

bool TofinoModuleFactory::is_compute_module(const Module *module) {
  return module->get_target() == TargetType::Tofino &&
         (module->get_type() == ModuleType::Tofino_ArithmeticOp || module->get_type() == ModuleType::Tofino_RotateLeft ||
          module->get_type() == ModuleType::Tofino_RotateLeftShifts);
}

namespace {

void push_unique(std::vector<DS_ID> &actions, const DS_ID &id) {
  if (std::find(actions.begin(), actions.end(), id) == actions.end()) {
    actions.push_back(id);
  }
}

void collect_ep_compute_run(const EP *ep, std::vector<DS_ID> &actions) {
  if (!ep->has_active_leaf()) {
    return;
  }
  const EPNode *ep_node = ep->get_active_leaf().node;
  while (ep_node && ep_node->get_module() && TofinoModuleFactory::is_compute_module(ep_node->get_module())) {
    const TofinoModule *module = dynamic_cast<const TofinoModule *>(ep_node->get_module());
    for (const DS_ID &id : module->get_generated_ds()) {
      push_unique(actions, id);
    }
    ep_node = ep_node->get_prev();
  }
}

} // namespace

std::vector<DS_ID> TofinoModuleFactory::get_compute_run_actions(const EP *ep) {
  std::vector<DS_ID> actions;
  collect_ep_compute_run(ep, actions);
  return actions;
}

std::vector<DS_ID> TofinoModuleFactory::get_compute_run_actions(const EP *ep, const speculations_t &speculations) {
  std::vector<DS_ID> actions(speculations.run_actions.rbegin(), speculations.run_actions.rend());
  if (speculations.run_reaches_leaf) {
    collect_ep_compute_run(ep, actions);
  }
  return actions;
}

std::optional<DS_ID> TofinoModuleFactory::ComputeStepBuilder::place(const compute_op_t &op, std::unordered_set<DS_ID> deps) {
  if (new_pass) {
    std::erase_if(deps, [this](const DS_ID &dep) { return std::find(actions.begin(), actions.end(), dep) == actions.end(); });
  }

  std::vector<DS_ID> candidates(actions.rbegin(), actions.rend());
  candidates.insert(candidates.end(), run.begin(), run.end());

  const DS_ID new_action_id = "compute_" + op.id;
  const std::optional<TofinoContext::compute_op_plan_t> plan = ctx->plan_compute_op(candidates, new_action_id, op, deps);

  if (plan && plan->append) {
    ctx->append_compute_op(plan->action_id, op, deps);
    push_unique(actions, plan->action_id);
    if (!full_placer) {
      GlobalStats::num_spec_compute_appended++;
    }
    return plan->action_id;
  }

  ComputeAction *action = new ComputeAction(new_action_id, node->get_id(), {op});
  const bool fits       = full_placer ? ctx->can_place(action, deps) : (plan.has_value() && !plan->append);
  if (!fits) {
    delete action;
    return {};
  }

  ctx->place(node->get_id(), action, deps);
  actions.push_back(new_action_id);
  if (!full_placer) {
    GlobalStats::num_spec_compute_new_action++;
  }
  return new_action_id;
}

std::optional<TofinoModuleFactory::compute_step_t> TofinoModuleFactory::implement_compute_step(const EP *ep, const BDDNode *node,
                                                                                               const compute_step_builder_fn_t &build) const {
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);
  TofinoContext *tofino_ctx  = new_ep->get_mutable_ctx().get_mutable_target_ctx<TofinoContext>();

  ComputeStepBuilder builder{.node = node, .ctx = tofino_ctx, .run = get_compute_run_actions(ep), .full_placer = true, .new_pass = false, .actions = {}};
  const std::optional<DS_ID> out = build(builder);
  if (!out) {
    return {};
  }

  return compute_step_t{std::move(new_ep), *out};
}

std::optional<spec_impl_t> TofinoModuleFactory::speculate_compute_step(const EP *ep, const BDDNode *node, const compute_step_builder_fn_t &build,
                                                                       const speculations_t &speculations) const {
  struct attempt_t {
    Context ctx;
    std::vector<DS_ID> actions;
    DS_ID out;
  };

  const auto attempt = [&](bool new_pass) -> std::optional<attempt_t> {
    Context new_ctx           = speculations.ctx;
    TofinoContext *tofino_ctx = new_ctx.get_mutable_target_ctx<TofinoContext>();
    ComputeStepBuilder builder{.node        = node,
                               .ctx         = tofino_ctx,
                               .run         = new_pass ? std::vector<DS_ID>{} : get_compute_run_actions(ep, speculations),
                               .full_placer = false,
                               .new_pass    = new_pass,
                               .actions     = {}};
    const std::optional<DS_ID> out = build(builder);
    if (!out) {
      return {};
    }
    return attempt_t{std::move(new_ctx), builder.actions, *out};
  };

  bool recirculated               = false;
  std::optional<attempt_t> result = attempt(false);

  if (!result) {
    // The pipeline is used up: the packet goes around once more, and the step waits for
    // nothing placed in the previous pass.
    if (ep->count_speculative_past_recirculations(node, speculations) > MAX_PAST_RECIRCULATIONS) {
      GlobalStats::num_spec_compute_cap_declined++;
      return {};
    }

    result = attempt(true);
    if (!result) {
      return {};
    }

    const hit_rate_t node_hr = result->ctx.get_profiler().get_hr(node);
    result->ctx.get_mutable_perf_oracle().add_recirculated_traffic(ep->get_speculative_node_egress(node_hr, node, speculations));
    recirculated = true;
    GlobalStats::num_spec_compute_recirculated++;
  }

  spec_impl_t spec_impl(decide(ep, node), result->ctx);
  spec_impl.recirculated    = recirculated;
  spec_impl.compute_step    = true;
  spec_impl.compute_actions = result->actions;
  if (const Call *call_node = dynamic_cast<const Call *>(node)) {
    for (const symbol_t &symbol : call_node->get_local_symbols().get()) {
      spec_impl.produced[symbol.name] = result->out;
    }
  }

  return spec_impl;
}

bool TofinoModuleFactory::reads_pending_write_borrow_value(const BDDNode *node, klee::ref<klee::Expr> expr) {
  const std::unordered_set<std::string> read = symbol_t::get_symbols_names(expr);

  for (const Call *vector_borrow : node->get_prev_functions({"vector_borrow"})) {
    if (!vector_borrow->is_vector_write() || !read.count(vector_borrow->get_local_symbol("vector_data").name)) {
      continue;
    }
    for (const Call *vector_return : vector_borrow->get_vector_returns_from_borrow()) {
      // is_reachable walks backwards: the return is still ahead iff `node` is one of its ancestors.
      if (vector_return->is_reachable(node->get_id())) {
        return true;
      }
    }
  }

  return false;
}

Symbols TofinoModuleFactory::get_relevant_dataplane_state(const EP *ep, const BDDNode *node) {
  const bdd_node_ids_t &roots = ep->get_target_roots(TargetType::Tofino);

  Symbols generated_symbols = node->get_prev_symbols(roots);
  generated_symbols.add(ep->get_bdd()->get_device());
  generated_symbols.add(ep->get_bdd()->get_time());

  Symbols future_used_symbols;
  node->visit_nodes([&future_used_symbols](const BDDNode *future_node) {
    const Symbols local_future_symbols = future_node->get_used_symbols();
    future_used_symbols.add(local_future_symbols);
    return BDDNodeVisitAction::Continue;
  });

  return generated_symbols.intersect(future_used_symbols);
}

void TofinoModuleFactory::speculate_sending_to_controller(const EP *ep, const BDDNode *node, Context &ctx, const speculations_t &speculations,
                                                          hit_rate_t relative_hr_sent_to_controller, bool local_recirculation_decision) {
  const Profiler &profiler       = ctx.get_profiler();
  const hit_rate_t node_hr       = profiler.get_hr(node);
  const hit_rate_t controller_hr = node_hr * relative_hr_sent_to_controller.value;

  port_ingress_t controller_node_egress = ep->get_speculative_node_egress(controller_hr, node, speculations, local_recirculation_decision);

  ctx.get_mutable_perf_oracle().add_controller_traffic(controller_node_egress);

  node->visit_nodes([&ctx, relative_hr_sent_to_controller, controller_node_egress](const BDDNode *future_node) {
    if (future_node->get_type() != BDDNodeType::Route) {
      return BDDNodeVisitAction::Continue;
    }

    const Route *route_node = dynamic_cast<const Route *>(future_node);

    const fwd_stats_t fwd_stats                       = ctx.get_profiler().get_fwd_stats(route_node);
    const std::unordered_set<u16> candidate_fwd_ports = ctx.get_profiler().get_candidate_fwd_ports(route_node);

    switch (fwd_stats.operation) {
    case RouteOp::Forward: {
      for (const u16 device : candidate_fwd_ports) {
        const hit_rate_t dev_hr = fwd_stats.ports.at(device) * relative_hr_sent_to_controller.value;
        if (dev_hr == 0_hr) {
          continue;
        }

        port_ingress_t node_egress;
        node_egress.controller = dev_hr;

        ctx.get_mutable_perf_oracle().add_fwd_traffic(device, node_egress);
      }
    } break;
    case RouteOp::Drop: {
      ctx.get_mutable_perf_oracle().add_controller_dropped_traffic(fwd_stats.drop * relative_hr_sent_to_controller.value);
    } break;
    case RouteOp::Broadcast: {
      for (const auto &[device, _] : fwd_stats.ports) {
        port_ingress_t node_egress;
        node_egress.controller = fwd_stats.ports.at(device) * relative_hr_sent_to_controller.value;
        ctx.get_mutable_perf_oracle().add_fwd_traffic(device, node_egress);
      }
    } break;
    }

    return BDDNodeVisitAction::Continue;
  });

  ctx.get_mutable_profiler().scale(node->get_ordered_branch_constraints(), (1_hr - relative_hr_sent_to_controller).value);
}

} // namespace Tofino
} // namespace LibSynapse