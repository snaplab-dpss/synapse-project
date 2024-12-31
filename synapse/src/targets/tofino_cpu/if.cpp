#include "if.h"
#include "else.h"
#include "then.h"

namespace tofino_cpu {

std::optional<spec_impl_t> IfGenerator::speculate(const EP *ep,
                                                  const Node *node,
                                                  const Context &ctx) const {
  if (node->get_type() != NodeType::Branch) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> IfGenerator::process_node(const EP *ep,
                                              const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Branch) {
    return impls;
  }

  const Branch *branch_node = static_cast<const Branch *>(node);

  klee::ref<klee::Expr> condition = branch_node->get_condition();

  ASSERT(branch_node->get_on_true(), "Branch node has no on_true");
  ASSERT(branch_node->get_on_false(), "Branch node has no on_false");

  Module *if_module = new If(node, condition);
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

} // namespace tofino_cpu
