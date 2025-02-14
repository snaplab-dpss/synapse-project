#include <LibSynapse/Modules/Tofino/If.h>
#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

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

std::optional<spec_impl_t> IfFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Branch) {
    return std::nullopt;
  }

  const LibBDD::Branch *branch_node = dynamic_cast<const LibBDD::Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> IfFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Branch) {
    return impls;
  }

  const LibBDD::Branch *branch_node = dynamic_cast<const LibBDD::Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return impls;
  }

  klee::ref<klee::Expr> condition = branch_node->get_condition();

  std::vector<klee::ref<klee::Expr>> conditions;
  for (klee::ref<klee::Expr> sub_condition : split_condition(condition)) {
    if (!get_tna(ep).condition_meets_phv_limit(sub_condition)) {
      panic("TODO: deal with this");
      return impls;
    }

    klee::ref<klee::Expr> simplified = LibCore::simplify_conditional(sub_condition);
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

std::unique_ptr<Module> IfFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Branch) {
    return {};
  }

  const LibBDD::Branch *branch_node = dynamic_cast<const LibBDD::Branch *>(node);

  if (branch_node->is_parser_condition()) {
    return {};
  }

  klee::ref<klee::Expr> condition    = branch_node->get_condition();
  const LibSynapse::Tofino::TNA &tna = ctx.get_target_ctx<TofinoContext>()->get_tna();

  std::vector<klee::ref<klee::Expr>> conditions;
  for (klee::ref<klee::Expr> sub_condition : split_condition(condition)) {
    if (!tna.condition_meets_phv_limit(sub_condition)) {
      panic("TODO: deal with this");
      return {};
    }

    klee::ref<klee::Expr> simplified = LibCore::simplify_conditional(sub_condition);
    conditions.push_back(simplified);
  }

  return std::make_unique<If>(node, condition, conditions);
}

} // namespace Tofino
} // namespace LibSynapse