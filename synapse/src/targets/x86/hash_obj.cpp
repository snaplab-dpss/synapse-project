#include "hash_obj.h"

namespace synapse {
namespace x86 {
namespace {
bool bdd_node_match_pattern(const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "hash_obj") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> HashObjFactory::speculate(const EP *ep, const Node *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> HashObjFactory::process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> obj_addr_expr = call.args.at("obj").expr;
  klee::ref<klee::Expr> size = call.args.at("size").expr;
  klee::ref<klee::Expr> hash = call.ret;

  addr_t obj_addr = expr_addr_to_obj_addr(obj_addr_expr);

  Module *module = new HashObj(node, obj_addr, size, hash);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace x86
} // namespace synapse