#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Branch;

using LibCore::expr_addr_to_obj_addr;
using LibCore::simplify_conditional;

namespace {
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
} // namespace

std::optional<spec_impl_t> IfFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> IfFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Branch) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return {};
  }

  klee::ref<klee::Expr> condition = branch_node->get_condition();

  std::vector<klee::ref<klee::Expr>> conditions;
  for (klee::ref<klee::Expr> sub_condition : split_condition(condition)) {
    if (!get_tna(ep).condition_meets_phv_limit(sub_condition)) {
      panic("TODO: deal with this");
      return {};
    }

    klee::ref<klee::Expr> simplified = simplify_conditional(sub_condition);
    conditions.push_back(simplified);
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

  std::vector<klee::ref<klee::Expr>> conditions;
  for (klee::ref<klee::Expr> sub_condition : split_condition(condition)) {
    if (!tna.condition_meets_phv_limit(sub_condition)) {
      panic("TODO: deal with this");
      return {};
    }

    klee::ref<klee::Expr> simplified = simplify_conditional(sub_condition);
    conditions.push_back(simplified);
  }

  return std::make_unique<If>(node, condition, conditions);
}

} // namespace Tofino
} // namespace LibSynapse