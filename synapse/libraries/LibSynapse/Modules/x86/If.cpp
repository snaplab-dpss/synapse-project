#include <LibSynapse/Modules/x86/If.h>
#include <LibSynapse/Modules/x86/Then.h>
#include <LibSynapse/Modules/x86/Else.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

namespace {
bool bdd_node_match_pattern(const LibBDD::Node *node) {
  if (node->get_type() != LibBDD::NodeType::Branch) {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> IfFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> IfFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Branch *branch_node = dynamic_cast<const LibBDD::Branch *>(node);

  klee::ref<klee::Expr> condition = branch_node->get_condition();

  assert(branch_node->get_on_true() && "Missing on_true");
  assert(branch_node->get_on_false() && "Missing on_false");

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
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Branch *branch_node = dynamic_cast<const LibBDD::Branch *>(node);
  klee::ref<klee::Expr> condition   = branch_node->get_condition();

  return std::make_unique<If>(node, condition);
}

} // namespace x86
} // namespace LibSynapse