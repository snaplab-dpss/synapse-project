#include <LibTessera/Modules/Tofino/TofinoModule.h>
#include <LibTessera/Modules/Tofino/TofinoContext.h>
#include <LibTessera/ExecutionPlan.h>

#include <unordered_set>
#include <klee/util/ExprVisitor.h>

namespace LibTessera {
namespace Tofino {

using LibBDD::Route;
using LibBDD::RouteOp;

namespace {
class ActionExprCompatibilityChecker : public klee::ExprVisitor::ExprVisitor {
private:
  std::unordered_set<std::string> symbols;
  int operations;
  bool compatible;

public:
  ActionExprCompatibilityChecker() : operations(0), compatible(true) {}

  bool is_compatible() const { return compatible; }

  Action visit_incompatible_op() {
    compatible = false;
    return Action::skipChildren();
  }

  Action visit_compatible_op() {
    operations++;
    if (operations > 1) {
      compatible = false;
      return Action::skipChildren();
    }
    return Action::doChildren();
  }

  Action visitRead(const klee::ReadExpr &e) override final {
    const std::string name = e.updates.root->name;
    symbols.insert(name);
    if (symbols.size() > 1) {
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
} // namespace LibTessera