#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/ExecutionPlan.h>

#include <LibCore/Math.h>
#include <LibCore/Expr.h>

#include <klee/util/ExprVisitor.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Branch;
using LibCore::expr_addr_to_obj_addr;
using LibCore::is_power_of_two;
using LibCore::is_readLSB;
using LibCore::simplify_conditional;
using LibCore::symbolic_read_t;
using LibCore::symbolic_reads_t;

namespace {

class PHVBytesRetriever : public klee::ExprVisitor::ExprVisitor {
private:
  symbolic_reads_t symbolic_reads;
  bytes_t used_phv_bytes;

public:
  PHVBytesRetriever() : used_phv_bytes(0) {}

  Action visitRead(const klee::ReadExpr &e) override final {
    assert(e.index->getKind() == klee::Expr::Kind::Constant && "Non-constant index");

    const klee::ConstantExpr *index_const = dynamic_cast<klee::ConstantExpr *>(e.index.get());
    const bytes_t byte                    = index_const->getZExtValue();
    const std::string name                = e.updates.root->name;
    const symbolic_read_t symbolic_read{byte, name};

    if (!symbolic_reads.contains(symbolic_read)) {
      symbolic_reads.insert({byte, name});
      used_phv_bytes += 1;
    }

    return Action::doChildren();
  }

  Action visitExtract(const klee::ExtractExpr &e) override final {
    // A bit-slice only consumes the extracted bytes, regardless of how wide the
    // underlying value is. Don't recurse into the child (which is the full value).
    used_phv_bytes += (e.width + 7) / 8;
    return Action::skipChildren();
  }

  Action visitExpr(const klee::Expr &e) override final {
    if (e.getKind() == klee::Expr::Kind::Read) {
      return Action::doChildren();
    }

    for (size_t i = 0; i < e.getNumKids(); i++) {
      klee::ref<klee::Expr> kid = e.getKid(i);
      if (kid->getKind() == klee::Expr::Constant) {
        const u64 value = solver_toolbox.value_from_expr(kid);

        // If the value is 1 less than a power of two, the compiler can optimize it to not consume PHV resources.
        if (is_power_of_two(value + 1)) {
          continue;
        }

        const bytes_t width = kid->getWidth() / 8;
        used_phv_bytes += width;
      }
    }

    return Action::doChildren();
  }

  bytes_t get_used_phv_bytes() const { return used_phv_bytes; }
};

std::vector<klee::ref<klee::Expr>> split_condition(klee::ref<klee::Expr> condition) {
  std::vector<klee::ref<klee::Expr>> conditions;

  switch (condition->getKind()) {
  case klee::Expr::Kind::And: {
    klee::ref<klee::Expr> lhs = condition->getKid(0);
    klee::ref<klee::Expr> rhs = condition->getKid(1);

    std::vector<klee::ref<klee::Expr>> lhs_conds = split_condition(lhs);
    std::vector<klee::ref<klee::Expr>> rhs_conds = split_condition(rhs);

    conditions.insert(conditions.end(), lhs_conds.begin(), lhs_conds.end());
    conditions.insert(conditions.end(), rhs_conds.begin(), rhs_conds.end());
  } break;
  case klee::Expr::Kind::Or: {
    panic("TODO: Splitting if condition on an OR");
  } break;
  default: {
    conditions.push_back(condition);
  }
  }

  return conditions;
}

std::optional<If::phv_limitation_workaround_t> get_phv_limitation_workaround(klee::ref<klee::Expr> expr) {
  static const std::map<std::pair<klee::Expr::Width, klee::Expr::Kind>, If::ConditionActionHelper> kind_to_action_helper{
      {{klee::Expr::Int32, klee::Expr::Kind::Eq}, If::ConditionActionHelper::None},
      {{klee::Expr::Int32, klee::Expr::Kind::Ne}, If::ConditionActionHelper::None},
      {{klee::Expr::Int32, klee::Expr::Kind::Sle}, If::ConditionActionHelper::CheckSignBitForLessThanOrEqual32b},
      {{klee::Expr::Int32, klee::Expr::Kind::Slt}, If::ConditionActionHelper::CheckSignBitForLessThan32b},
      {{klee::Expr::Int32, klee::Expr::Kind::Sge}, If::ConditionActionHelper::CheckSignBitForGreaterThanOrEqual32b},
      {{klee::Expr::Int32, klee::Expr::Kind::Sgt}, If::ConditionActionHelper::CheckSignBitForGreaterThan32b},
  };

  if (expr->getKind() == klee::Expr::Kind::Not) {
    expr = expr->getKid(0);
  }

  if (expr->getNumKids() != 2) {
    return {};
  }

  const klee::Expr::Width width = expr->getKid(0)->getWidth();
  const klee::Expr::Kind kind   = expr->getKind();

  auto found_it = kind_to_action_helper.find({width, kind});
  if (found_it == kind_to_action_helper.end()) {
    return {};
  }

  return If::phv_limitation_workaround_t(found_it->second, expr->getKid(0), expr->getKid(1));
}

// Rewrite an unsigned comparison-with-constant that exceeds the gateway PHV limit
// into an equivalent expression built from narrow slices, so it fits. Returns the
// rewritten (equivalent) condition, or nullopt when no OR-free rewrite exists
// (multi-chunk constant, non-constant rhs, or a direction that would need an OR).
//   a < C, C < 2^N and a = ZExt(y:N)  -> y < C          (operand already narrow)
//   a < C, C < 2^k <= width(a)        -> a[hi]==0 && a[lo] < C   (guarded slice)
std::optional<klee::ref<klee::Expr>> rewrite_constant_comparison(klee::ref<klee::Expr> expr) {
  const klee::Expr::Kind kind = expr->getKind();
  if (kind != klee::Expr::Kind::Ult && kind != klee::Expr::Kind::Ule) {
    return {};
  }

  klee::ref<klee::Expr> lhs = expr->getKid(0);
  klee::ref<klee::Expr> rhs = expr->getKid(1);

  const klee::ConstantExpr *constant = dynamic_cast<klee::ConstantExpr *>(rhs.get());
  if (!constant || dynamic_cast<klee::ConstantExpr *>(lhs.get())) {
    return {};
  }

  const u64 c = constant->getZExtValue();
  auto rebuild = [&](klee::ref<klee::Expr> l, klee::ref<klee::Expr> r) {
    return kind == klee::Expr::Kind::Ult ? solver_toolbox.exprBuilder->Ult(l, r) : solver_toolbox.exprBuilder->Ule(l, r);
  };

  // The operand is already a zero-extension of a narrow value: compare the narrow
  // value directly against a same-width constant, no guard needed.
  if (lhs->getKind() == klee::Expr::Kind::ZExt) {
    klee::ref<klee::Expr> inner = lhs->getKid(0);
    const bits_t n              = inner->getWidth();
    if (n < 64 && c < (1ull << n)) {
      return rebuild(inner, solver_toolbox.exprBuilder->Constant(c, n));
    }
    return {};
  }

  // Genuinely wide operand: slice at the smallest byte boundary that contains C,
  // guard that the high part is zero, and compare the low part.
  const bits_t w = lhs->getWidth();
  bits_t k       = 8;
  while (k < w && (c >> k) != 0) {
    k += 8;
  }
  if (k >= w) {
    return {};
  }

  klee::ref<klee::Expr> high      = solver_toolbox.exprBuilder->Extract(lhs, k, w - k);
  klee::ref<klee::Expr> low       = solver_toolbox.exprBuilder->Extract(lhs, 0, k);
  klee::ref<klee::Expr> high_zero = solver_toolbox.exprBuilder->Eq(high, solver_toolbox.exprBuilder->Constant(0, w - k));
  klee::ref<klee::Expr> low_cmp   = rebuild(low, solver_toolbox.exprBuilder->Constant(c, k));

  return solver_toolbox.exprBuilder->And(high_zero, low_cmp);
}

} // namespace

// Collect the topmost arithmetic sub-expressions of `expr` (Add/Sub/Mul/*Div/*Rem). A
// Tofino gateway can compare fields, constants and bit-slices, but cannot evaluate
// arithmetic; such operands must be materialized into metadata before the gateway.
void collect_materializable_operands(klee::ref<klee::Expr> expr, std::vector<klee::ref<klee::Expr>> &out) {
  if (expr.isNull()) {
    return;
  }
  switch (expr->getKind()) {
  case klee::Expr::Add:
  case klee::Expr::Sub:
  case klee::Expr::Mul:
  case klee::Expr::UDiv:
  case klee::Expr::SDiv:
  case klee::Expr::URem:
  case klee::Expr::SRem:
    out.push_back(expr); // materialize the whole arithmetic operand; don't recurse in
    return;
  default:
    break;
  }
  for (unsigned i = 0; i < expr->getNumKids(); i++) {
    collect_materializable_operands(expr->getKid(i), out);
  }
}

std::vector<If::condition_t> IfFactory::get_compatible_conditions(const TNA &tna, klee::ref<klee::Expr> condition) {
  std::vector<If::condition_t> conditions;

  for (klee::ref<klee::Expr> sub_condition : split_condition(condition)) {
    klee::ref<klee::Expr> simplified = simplify_conditional(sub_condition);

    if (!tna.is_simple_conditional_expr(simplified)) {
      return {};
    }

    if (tna.condition_meets_phv_limit(simplified)) {
      If::condition_t cond(simplified);
      collect_materializable_operands(simplified, cond.operands_to_materialize);
      conditions.push_back(cond);
      continue;
    }

    // Doesn't fit a gateway. First try to rewrite a constant comparison into an
    // equivalent expression of narrow slices, then feed that back through the
    // same pipeline (split_condition + re-validation) so the If module ends up
    // with ready-to-emit sub-conditions.
    if (std::optional<klee::ref<klee::Expr>> rewritten = rewrite_constant_comparison(simplified)) {
      std::vector<If::condition_t> sub = get_compatible_conditions(tna, *rewritten);
      if (sub.empty()) {
        return {};
      }
      conditions.insert(conditions.end(), sub.begin(), sub.end());
      continue;
    }

    // Fall back to the legacy signed sign-bit render-helper (single gateway).
    if (std::optional<If::phv_limitation_workaround_t> phv_limitation_workaround = get_phv_limitation_workaround(simplified)) {
      conditions.push_back({simplified, *phv_limitation_workaround});
      continue;
    }

    // Cannot lower this condition on Tofino: decline so the search backtracks.
    return {};
  }

  return conditions;
}

std::optional<spec_impl_t> IfFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return {};
  }

  // Decline branches whose condition can't be lowered onto a Tofino gateway.
  if (get_compatible_conditions(get_tna(ep), branch_node->get_condition()).empty()) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> IfFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return {};
  }

  klee::ref<klee::Expr> condition               = branch_node->get_condition();
  const std::vector<If::condition_t> conditions = get_compatible_conditions(get_tna(ep), condition);

  if (conditions.empty()) {
    return {};
  }

  assert(branch_node->get_on_true() && "Branch node without on_true");
  assert(branch_node->get_on_false() && "Branch node without on_false");

  Module *if_module   = new If(node, condition, conditions);
  Module *then_module = new Then(node);
  Module *else_module = new Else(node);

  EPNode *if_node   = new EPNode(if_module);
  EPNode *then_node = new EPNode(then_module);
  EPNode *else_node = new EPNode(else_module);

  if_node->set_children(condition, then_node, else_node);
  then_node->set_prev(if_node);
  else_node->set_prev(if_node);

  EPLeaf then_leaf(then_node, branch_node->get_on_true());
  EPLeaf else_leaf(else_node, branch_node->get_on_false());

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_ep->process_leaf(if_node, {then_leaf, else_leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> IfFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return {};
  }

  klee::ref<klee::Expr> condition = branch_node->get_condition();
  const Tofino::TNA &tna          = ctx.get_target_ctx<TofinoContext>()->get_tna();

  const std::vector<If::condition_t> conditions = get_compatible_conditions(tna, condition);

  return std::make_unique<If>(node, condition, conditions);
}

} // namespace Tofino
} // namespace LibSynapse