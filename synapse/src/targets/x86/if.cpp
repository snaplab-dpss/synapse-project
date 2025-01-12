#include "if.h"
#include "else.h"
#include "then.h"

namespace synapse {
namespace x86 {
namespace {
bool bdd_node_match_pattern(const Node *node) {
  if (node->get_type() != NodeType::Branch) {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> IfFactory::speculate(const EP *ep, const Node *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> IfFactory::process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  klee::ref<klee::Expr> condition = branch_node->get_condition();

  assert(branch_node->get_on_true() && "Missing on_true");
  assert(branch_node->get_on_false() && "Missing on_false");

  Module *if_module   = new If(node, condition);
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

} // namespace x86
} // namespace synapse