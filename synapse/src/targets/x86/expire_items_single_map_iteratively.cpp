#include "expire_items_single_map_iteratively.h"

namespace x86 {

namespace {
bool bdd_node_match_pattern(const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "expire_items_single_map_iteratively") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ExpireItemsSingleMapIterativelyGenerator::speculate(
    const EP *ep, const Node *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t>
ExpireItemsSingleMapIterativelyGenerator::process_node(const EP *ep,
                                                       const Node *node) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> start = call.args.at("start").expr;
  klee::ref<klee::Expr> n_elems = call.args.at("n_elems").expr;

  addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);
  addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

  Module *module = new ExpireItemsSingleMapIteratively(
      node, map_addr, vector_addr, start, n_elems);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace x86
