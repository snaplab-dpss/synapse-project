#include "expire_items_single_map.h"

namespace synapse {
namespace x86 {
namespace {
bool bdd_node_match_pattern(const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "expire_items_single_map") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ExpireItemsSingleMapFactory::speculate(const EP *ep, const Node *node,
                                                                  const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> ExpireItemsSingleMapFactory::process_node(const EP *ep, const Node *node,
                                                              SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> dchain = call.args.at("chain").expr;
  klee::ref<klee::Expr> vector = call.args.at("vector").expr;
  klee::ref<klee::Expr> map = call.args.at("map").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;
  klee::ref<klee::Expr> total_freed = call.ret;

  addr_t map_addr = expr_addr_to_obj_addr(map);
  addr_t vector_addr = expr_addr_to_obj_addr(vector);
  addr_t dchain_addr = expr_addr_to_obj_addr(dchain);

  Module *module =
      new ExpireItemsSingleMap(node, dchain_addr, vector_addr, map_addr, time, total_freed);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace x86
} // namespace synapse