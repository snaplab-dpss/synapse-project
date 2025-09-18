#include <LibSynapse/Modules/x86/If.h>
#include <LibSynapse/Modules/x86/Then.h>
#include <LibSynapse/Modules/x86/Else.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Branch;

using LibCore::expr_addr_to_obj_addr;

namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Branch) {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> IfFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return {};
}

std::vector<impl_t> IfFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Branch *branch_node = dynamic_cast<const Branch *>(node);

  klee::ref<klee::Expr> condition = branch_node->get_condition();

  assert(branch_node->get_on_true() && "Missing on_true");
  assert(branch_node->get_on_false() && "Missing on_false");

  Module *if_module   = new If(get_type().instance_id, node, condition);
  Module *then_module = new Then(get_type().instance_id, node);
  Module *else_module = new Else(get_type().instance_id, node);

  EPNode *if_node   = new EPNode(if_module);
  EPNode *then_node = new EPNode(then_module);
  EPNode *else_node = new EPNode(else_module);

  if_node->set_children(condition, then_node, else_node);
  then_node->set_prev(if_node);
  else_node->set_prev(if_node);

  const EPLeaf then_leaf(then_node, branch_node->get_on_true());
  const EPLeaf else_leaf(else_node, branch_node->get_on_false());

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_ep->process_leaf(if_node, {then_leaf, else_leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> IfFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Branch *branch_node       = dynamic_cast<const Branch *>(node);
  klee::ref<klee::Expr> condition = branch_node->get_condition();

  return std::make_unique<If>(get_type().instance_id, node, condition);
}

} // namespace x86
} // namespace LibSynapse