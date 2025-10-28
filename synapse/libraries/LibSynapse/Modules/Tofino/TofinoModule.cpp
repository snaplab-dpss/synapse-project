#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <cassert>
#include <unordered_set>
#include <klee/util/ExprVisitor.h>

namespace LibSynapse {
namespace Tofino {

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

bool TofinoModuleFactory::was_ds_already_used(const EPNode *node, DS_ID ds_id) const {
  while (node) {
    if (node->get_module()->get_target().type == TargetArchitecture::Tofino) {
      const TofinoModule *tofino_module = dynamic_cast<const TofinoModule *>(node->get_module());

      if (tofino_module->get_type().type == ModuleCategory::Tofino_Recirculate) {
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

TofinoContext *TofinoModuleFactory::get_mutable_tofino_ctx(EP *ep, TargetType target) {
  assert(target.type == TargetArchitecture::Tofino && "Invalid Target Architecture");
  Context &ctx = ep->get_mutable_ctx();
  return ctx.get_mutable_target_ctx<TofinoContext>(target);
}

const TofinoContext *TofinoModuleFactory::get_tofino_ctx(const EP *ep, TargetType target) {
  assert(target.type == TargetArchitecture::Tofino && "Invalid Target Architecture");
  const Context &ctx = ep->get_ctx();
  return ctx.get_target_ctx<TofinoContext>(target);
}

TNA &TofinoModuleFactory::get_mutable_tna(EP *ep, const TargetType target) {
  TofinoContext *ctx = get_mutable_tofino_ctx(ep, target);
  return ctx->get_mutable_tna();
}

const TNA &TofinoModuleFactory::get_tna(const EP *ep, const TargetType target) {
  const TofinoContext *ctx = get_tofino_ctx(ep, target);
  return ctx->get_tna();
}

bool TofinoModuleFactory::expr_fits_in_action(klee::ref<klee::Expr> expr) {
  ActionExprCompatibilityChecker checker;
  checker.visit(expr);
  return checker.is_compatible();
}

Symbols TofinoModuleFactory::get_relevant_dataplane_state(const EP *ep, const BDDNode *node, const TargetType type) {
  assert(type.type == TargetArchitecture::Tofino && "Ilegal Target");
  const bdd_node_ids_t &roots = ep->get_target_roots(type);

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

} // namespace Tofino
} // namespace LibSynapse
