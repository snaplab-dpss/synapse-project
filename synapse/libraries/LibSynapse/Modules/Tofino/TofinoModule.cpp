#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/GlobalStats.h>
#include <LibSynapse/Modules/Tofino/ArithmeticOp.h>
#include <LibSynapse/Modules/Tofino/RotateLeft.h>
#include <LibSynapse/Modules/Tofino/RotateLeftShifts.h>
#include <LibBDD/Unroll.h>
#include <LibCore/Expr.h>
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


namespace {
// Whether the pass this node belongs to has already crossed into the egress. Read off state that
// already exists -- the plan's own nodes, and the module type each speculation records -- so the
// generic structures need nothing Tofino-specific added to them.
bool crossed_this_pass(const EP *ep, const BDDNode *node, const speculations_t &speculations) {
  // The two walks cover different stretches of time and have to be consulted in that order. The
  // speculated steps come after the plan's own nodes, so they are asked first: a recirculation the
  // lookahead predicts ends the pass exactly as a placed one does, and the pass after it is
  // entitled to its own crossing. Asking the plan first says "already crossed" for the whole
  // speculation once the plan contains one crossing, which denies every later pass a free egress
  // and made the lookahead predict five recirculations where it should have predicted one.
  for (const BDDNode *n = node; n; n = n->get_prev()) {
    auto found = std::find_if(speculations.speculations_per_node.begin(), speculations.speculations_per_node.end(),
                              [n](const spec_impl_lite_t &spec) { return spec.decision.node == n->get_id(); });
    if (found == speculations.speculations_per_node.end()) {
      continue;
    }
    if (found->recirculated) {
      return false;
    }
    if (found->fresh_context) {
      return true;
    }
  }

  for (const EPNode *prev = ep->get_active_leaf().node; prev; prev = prev->get_prev()) {
    const Module *module = prev->get_module();
    if (!module || module->get_target() != TargetType::Tofino) {
      break;
    }
    if (module->get_type() == ModuleType::Tofino_SendToEgress) {
      return true;
    }
    if (module->get_type() == ModuleType::Tofino_Recirculate) {
      return false;
    }
  }

  return false;
}
} // namespace

bool TofinoModuleFactory::was_ds_already_used(const EPNode *node, DS_ID ds_id) {
  while (node) {
    if (node->get_module()->get_target() == TargetType::Tofino) {
      const TofinoModule *tofino_module = dynamic_cast<const TofinoModule *>(node->get_module());

      if (tofino_module->get_type() == ModuleType::Tofino_Recirculate || tofino_module->get_type() == ModuleType::Tofino_SendToEgress) {
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

bool is_run_transparent(ModuleType type) { return type == ModuleType::Tofino_Ignore; }

// Ignored nodes do nothing in the data plane: a reordering may interleave them with the steps
// of a run without breaking it.
void collect_ep_compute_run_from(const EPNode *ep_node, std::vector<DS_ID> &actions) {
  while (ep_node && ep_node->get_module() &&
         (TofinoModuleFactory::is_compute_module(ep_node->get_module()) || is_run_transparent(ep_node->get_module()->get_type()))) {
    if (TofinoModuleFactory::is_compute_module(ep_node->get_module())) {
      const TofinoModule *module = dynamic_cast<const TofinoModule *>(ep_node->get_module());
      for (const DS_ID &id : module->get_generated_ds()) {
        push_unique(actions, id);
      }
    }
    ep_node = ep_node->get_prev();
  }
}

void collect_ep_compute_run(const EP *ep, std::vector<DS_ID> &actions) {
  if (!ep->has_active_leaf()) {
    return;
  }
  collect_ep_compute_run_from(ep->get_active_leaf().node, actions);
}

} // namespace

bool TofinoModuleFactory::is_plain_operand(klee::ref<klee::Expr> expr) { return LibCore::is_constant(expr) || LibCore::is_readLSB(expr); }

std::vector<TofinoModuleFactory::compute_operand_t>
TofinoModuleFactory::get_operands_to_compute(const std::string &op_id_base, const std::vector<std::pair<std::string, klee::ref<klee::Expr>>> &exprs) {
  std::vector<compute_operand_t> operands;
  for (const auto &[suffix, expr] : exprs) {
    if (!is_plain_operand(expr)) {
      operands.push_back({expr, op_id_base + suffix, ""});
    }
  }
  return operands;
}

bool TofinoModuleFactory::place_operand_ops(ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, std::vector<compute_operand_t> &operands,
                                            const speculations_t *speculations) {
  for (compute_operand_t &operand : operands) {
    const std::unordered_set<DS_ID> deps = speculations ? TofinoContext::get_dataflow_deps(ep, node, operand.expr, *speculations)
                                                        : TofinoContext::get_dataflow_deps(ep, node, operand.expr);
    const std::optional<DS_ID> action =
        builder.place({.id = operand.op_id, .kind = ComputeOpKind::ALU, .width = operand.expr->getWidth()}, deps);
    if (!action) {
      return false;
    }
    operand.action_id = *action;
  }
  return true;
}

std::unordered_set<DS_ID> TofinoModuleFactory::get_op_deps(const EP *ep, const BDDNode *node, const std::vector<klee::ref<klee::Expr>> &plain,
                                                           const std::vector<compute_operand_t> &computed, const speculations_t *speculations) {
  std::unordered_set<DS_ID> deps;
  for (const klee::ref<klee::Expr> &expr : plain) {
    const std::unordered_set<DS_ID> d =
        speculations ? TofinoContext::get_dataflow_deps(ep, node, expr, *speculations) : TofinoContext::get_dataflow_deps(ep, node, expr);
    deps.insert(d.begin(), d.end());
  }
  for (const compute_operand_t &operand : computed) {
    deps.insert(operand.action_id);
  }
  return deps;
}

std::vector<DS_ID> TofinoModuleFactory::get_compute_run_actions(const EP *ep) {
  std::vector<DS_ID> actions;
  collect_ep_compute_run(ep, actions);
  return actions;
}

std::vector<DS_ID> TofinoModuleFactory::get_compute_run_actions(const EP *ep, const BDDNode *node, const speculations_t &speculations) {
  // Up the node's own path: the pass's compute steps (nearest first), then the plan's from the
  // leaf of the node's branch, until anything that is not a compute step or transparent, or a
  // recirculation.
  std::vector<DS_ID> actions;
  for (const BDDNode *n = node->get_prev(); n; n = n->get_prev()) {
    const speculations_t::node_info_t *info = speculations.find_node_info(n->get_id());
    if (!info) {
      collect_ep_compute_run_from(ep->get_leaf_ep_node_from_bdd_node(node), actions);
      break;
    }
    if (!info->compute_step && !is_run_transparent(info->module)) {
      break;
    }
    for (const DS_ID &action : info->actions) {
      push_unique(actions, action);
    }
    if (info->recirculated) {
      break;
    }
  }
  return actions;
}

std::optional<DS_ID> TofinoModuleFactory::ComputeStepBuilder::place(const compute_op_t &op, std::unordered_set<DS_ID> deps) {
  auto placed_it = placed_ops.find(op.id);
  if (placed_it != placed_ops.end()) {
    return placed_it->second;
  }

  if (new_pass) {
    std::erase_if(deps, [this](const DS_ID &dep) { return std::find(actions.begin(), actions.end(), dep) == actions.end(); });
  }

  // A gress only takes so much of a long computation before bf-p4c can no longer lay it out --
  // measured, not a PHV capacity limit, which is barely touched when it gives up. Refusing here
  // sends the compute-run ladder to its next option: the egress, which is a second budget, and
  // only then another lap, which is not.
  Pipeline &pipeline = ctx->get_mutable_tna().pipeline;
  if (!new_gress && !pipeline.compute_op_fits()) {
    return {};
  }

  std::vector<DS_ID> candidates(actions.rbegin(), actions.rend());
  candidates.insert(candidates.end(), run.begin(), run.end());

  const DS_ID new_action_id = "compute_" + op.id;
  const std::optional<TofinoContext::compute_op_plan_t> plan = ctx->plan_compute_op(candidates, new_action_id, op, deps);

  if (plan && plan->append) {
    ctx->append_compute_op(plan->action_id, op, deps);
    pipeline.charge_compute_op();
    push_unique(actions, plan->action_id);
    placed_ops.insert({op.id, plan->action_id});
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
  pipeline.charge_compute_op();
  actions.push_back(new_action_id);
  placed_ops.insert({op.id, new_action_id});
  if (!full_placer) {
    GlobalStats::num_spec_compute_new_action++;
  }
  return new_action_id;
}

bool TofinoModuleFactory::is_compute_node(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  const LibBDD::call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return LibBDD::is_unrolled_op(call) || call.function_name == "rotate_left";
}

namespace {

// Whether placing the step `node` computes places ops of its own before its op (operands that
// aren't plain values), which a failed step would leave behind.
bool step_has_computed_operands(const BDDNode *node) {
  const LibBDD::call_t &call = dynamic_cast<const Call *>(node)->get_call();
  if (LibBDD::is_unrolled_op(call)) {
    klee::ref<klee::Expr> value = LibBDD::unrolled_op_value(call);
    for (unsigned i = 0; i < value->getNumKids(); i++) {
      if (!TofinoModuleFactory::is_plain_operand(value->getKid(i))) {
        return true;
      }
    }
    return false;
  }
  return !TofinoModuleFactory::is_plain_operand(call.args.at("x").expr);
}

// Places the compute step `node` with the module the search would pick first for it: the
// arithmetic op, or the hash-unit rotate with the shift form as fallback.
std::optional<DS_ID> place_compute_node(TofinoModuleFactory::ComputeStepBuilder &builder, const EP *ep, const BDDNode *node,
                                        const speculations_t *speculations) {
  if (std::optional<DS_ID> out = ArithmeticOpFactory::place(builder, ep, node, speculations)) {
    return out;
  }
  if (std::optional<DS_ID> out = RotateLeftFactory::place(builder, ep, node, speculations)) {
    return out;
  }
  return RotateLeftShiftsFactory::place(builder, ep, node, speculations);
}

} // namespace

std::optional<spec_impl_t> TofinoModuleFactory::speculate_compute_run(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  struct attempt_t {
    Context ctx;
    std::vector<DS_ID> actions;
    std::unordered_map<std::string, std::string> produced;
    bdd_node_ids_t skip;
  };

  const auto attempt = [&](bool new_pass, bool new_gress = false) -> std::optional<attempt_t> {
    Context new_ctx           = speculations.ctx;
    TofinoContext *tofino_ctx = new_ctx.get_mutable_target_ctx<TofinoContext>();

    // The egress is a second pipeline with a budget of its own; a recirculation is not, since
    // bf-p4c lays out the one gress as a whole. Crossing moves the charge, it does not clear it.
    if (new_gress) {
      tofino_ctx->get_mutable_tna().pipeline.cross_to_egress();
    } else if (new_pass) {
      // A new pass that is not a crossing is a recirculation: back through the ingress.
      tofino_ctx->get_mutable_tna().pipeline.back_to_ingress();
    }
    ComputeStepBuilder builder{.node        = node,
                               .ctx         = tofino_ctx,
                               .run         = new_pass ? std::vector<DS_ID>{} : get_compute_run_actions(ep, node, speculations),
                               .full_placer = false,
                               .new_pass    = new_pass,
                               .new_gress   = new_gress,
                               .actions     = {},
                               .placed_ops  = {}};

    // The steps of the run see the ones placed before them (run_specs notes each as it goes).
    speculations_t run_specs = speculations;
    attempt_t result{std::move(new_ctx), {}, {}, {}};
    for (const BDDNode *n = node; n && is_compute_node(n); n = n->get_next()) {
      builder.node = n;

      // A step whose operand ops fit but whose own op doesn't must leave nothing behind: the
      // op is speculated again on its own, in a pass where the operands go elsewhere.
      std::optional<Context> ctx_before_step;
      std::vector<DS_ID> actions_before_step;
      std::unordered_map<std::string, DS_ID> placed_ops_before_step;
      if (step_has_computed_operands(n)) {
        ctx_before_step        = result.ctx;
        actions_before_step    = builder.actions;
        placed_ops_before_step = builder.placed_ops;
      }

      const std::optional<DS_ID> out = place_compute_node(builder, ep, n, &run_specs);
      if (!out) {
        if (ctx_before_step) {
          result.ctx         = std::move(*ctx_before_step);
          builder.ctx        = result.ctx.get_mutable_target_ctx<TofinoContext>();
          builder.actions    = std::move(actions_before_step);
          builder.placed_ops = std::move(placed_ops_before_step);
        }
        if (n == node) {
          return {};
        }
        break; // The run stops here; this op recirculates when it is speculated on its own.
      }
      speculations_t::node_info_t info{ModuleType::Tofino_ArithmeticOp, true, new_pass && n == node, builder.actions, {}};
      for (const symbol_t &symbol : dynamic_cast<const Call *>(n)->get_local_symbols().get()) {
        result.produced[symbol.name] = *out;
        info.produced[symbol.name]   = *out;
      }
      run_specs.note(n->get_id(), info);
      if (n != node) {
        result.skip.insert(n->get_id());
      }
    }
    result.actions = builder.actions;
    return result;
  };

  bool recirculated               = false;
  bool crossed                    = false;
  std::optional<attempt_t> result = attempt(false);

  if (!result && !crossed_this_pass(ep, node, speculations)) {
    // The ingress is used up, but the egress is a second pipeline on the same pass: the same
    // fresh placement context a recirculation would give, without costing a lap.
    result  = attempt(true, /*new_gress=*/true);
    crossed = result.has_value();
  }

  if (!result) {
    // The pipeline is used up: the packet goes around once more, and the run waits for nothing
    // placed in the previous pass.
    if (ep->count_speculative_past_recirculations(node, speculations) > MAX_PAST_RECIRCULATIONS) {
      GlobalStats::num_spec_compute_cap_declined++;
      return {};
    }

    result = attempt(true);
    if (!result) {
      return {};
    }

    const hit_rate_t node_hr = result->ctx.get_profiler().get_hr(node);
    const port_ingress_t eg  = ep->get_speculative_node_egress(node_hr, node, speculations);
    result->ctx.get_mutable_perf_oracle().add_recirculated_traffic(eg);
    recirculated = true;
    GlobalStats::num_spec_compute_recirculated++;
  }

  spec_impl_t spec_impl(decide(ep, node), result->ctx);
  spec_impl.fresh_context   = recirculated || crossed;
  spec_impl.recirculated    = recirculated;
  spec_impl.compute_step    = true;
  spec_impl.compute_actions = result->actions;
  spec_impl.produced        = result->produced;
  spec_impl.skip            = result->skip;
  return spec_impl;
}

std::optional<TofinoModuleFactory::compute_step_t> TofinoModuleFactory::implement_compute_step(const EP *ep, const BDDNode *node,
                                                                                               const compute_step_builder_fn_t &build) const {
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);
  TofinoContext *tofino_ctx  = new_ep->get_mutable_ctx().get_mutable_target_ctx<TofinoContext>();

  ComputeStepBuilder builder{
      .node = node, .ctx = tofino_ctx, .run = get_compute_run_actions(ep), .full_placer = true, .new_pass = false, .new_gress = false, .actions = {}, .placed_ops = {}};
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

  const auto attempt = [&](bool new_pass, bool new_gress = false) -> std::optional<attempt_t> {
    Context new_ctx           = speculations.ctx;
    TofinoContext *tofino_ctx = new_ctx.get_mutable_target_ctx<TofinoContext>();

    // The egress is a second pipeline with a budget of its own; a recirculation is not, since
    // bf-p4c lays out the one gress as a whole. Crossing moves the charge, it does not clear it.
    if (new_gress) {
      tofino_ctx->get_mutable_tna().pipeline.cross_to_egress();
    } else if (new_pass) {
      // A new pass that is not a crossing is a recirculation: back through the ingress.
      tofino_ctx->get_mutable_tna().pipeline.back_to_ingress();
    }
    ComputeStepBuilder builder{.node        = node,
                               .ctx         = tofino_ctx,
                               .run         = new_pass ? std::vector<DS_ID>{} : get_compute_run_actions(ep, node, speculations),
                               .full_placer = false,
                               .new_pass    = new_pass,
                               .new_gress   = new_gress,
                               .actions     = {},
                               .placed_ops  = {}};
    const std::optional<DS_ID> out = build(builder);
    if (!out) {
      return {};
    }
    return attempt_t{std::move(new_ctx), builder.actions, *out};
  };

  bool recirculated               = false;
  bool crossed                    = false;
  std::optional<attempt_t> result = attempt(false);

  if (!result && !crossed_this_pass(ep, node, speculations)) {
    // A fresh placement context for free, in the egress, before paying for a lap.
    result  = attempt(true, /*new_gress=*/true);
    crossed = result.has_value();
  }

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
  spec_impl.fresh_context   = recirculated || crossed;
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