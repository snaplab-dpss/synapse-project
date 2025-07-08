#include <LibSynapse/Modules/Controller/If.h>
#include <LibSynapse/Modules/Controller/Then.h>
#include <LibSynapse/Modules/Controller/Else.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> IfFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Branch) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> IfFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Branch) {
    return {};
  }

  const LibBDD::Branch *branch_node = dynamic_cast<const LibBDD::Branch *>(node);

  klee::ref<klee::Expr> condition = branch_node->get_condition();

  assert(branch_node->get_on_true() && "Branch node has no on_true");
  assert(branch_node->get_on_false() && "Branch node has no on_false");

  Module *if_module   = new If(node, condition);
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

std::unique_ptr<Module> IfFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Branch) {
    return {};
  }

  const LibBDD::Branch *branch_node = dynamic_cast<const LibBDD::Branch *>(node);
  klee::ref<klee::Expr> condition   = branch_node->get_condition();

  return std::make_unique<If>(node, condition);
}

} // namespace Controller
} // namespace LibSynapse