#include "if.h"

#include "else.h"
#include "then.h"

namespace tofino {

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
    ASSERT(false, "TODO");
  } break;
  default: {
    conditions.push_back(condition);
  }
  }

  return conditions;
}
} // namespace

std::optional<spec_impl_t> IfFactory::speculate(const EP *ep, const Node *node,
                                                const Context &ctx) const {
  if (node->get_type() != NodeType::Branch) {
    return std::nullopt;
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (is_parser_condition(branch_node)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> IfFactory::process_node(const EP *ep, const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Branch) {
    return impls;
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  if (is_parser_condition(branch_node)) {
    return impls;
  }

  klee::ref<klee::Expr> condition = branch_node->get_condition();
  std::vector<klee::ref<klee::Expr>> conditions = split_condition(condition);

  for (klee::ref<klee::Expr> sub_condition : conditions) {
    if (!get_tna(ep).condition_meets_phv_limit(sub_condition)) {
      ASSERT(false, "TODO: deal with this");
      return impls;
    }
  }

  ASSERT(branch_node->get_on_true(), "Branch node without on_true");
  ASSERT(branch_node->get_on_false(), "Branch node without on_false");

  Module *if_module = new If(node, condition, conditions);
  Module *then_module = new Then(node);
  Module *else_module = new Else(node);

  EPNode *if_node = new EPNode(if_module);
  EPNode *then_node = new EPNode(then_module);
  EPNode *else_node = new EPNode(else_module);

  if_node->set_constraint(condition);
  if_node->set_children(then_node, else_node);
  then_node->set_prev(if_node);
  else_node->set_prev(if_node);

  EPLeaf then_leaf(then_node, branch_node->get_on_true());
  EPLeaf else_leaf(else_node, branch_node->get_on_false());

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  new_ep->process_leaf(if_node, {then_leaf, else_leaf});

  return impls;
}

} // namespace tofino
