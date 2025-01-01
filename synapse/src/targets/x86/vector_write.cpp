#include "vector_write.h"

namespace x86 {

namespace {
bool bdd_node_match_pattern(const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "vector_return") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> VectorWriteFactory::speculate(const EP *ep, const Node *node,
                                                         const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::x86_Vector)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> VectorWriteFactory::process_node(const EP *ep,
                                                     const Node *node) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr = call.args.at("value").expr;
  klee::ref<klee::Expr> value = call.args.at("value").in;

  addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);
  addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

  if (!ep->get_ctx().can_impl_ds(vector_addr, DSImpl::x86_Vector)) {
    return impls;
  }

  klee::ref<klee::Expr> original_value = get_original_vector_value(ep, node, vector_addr);
  std::vector<mod_t> changes = build_expr_mods(original_value, value);

  // Check the Ignore module.
  if (changes.empty()) {
    return impls;
  }

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *module = new VectorWrite(node, vector_addr, index, value_addr, changes);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(vector_addr, DSImpl::x86_Vector);

  return impls;
}

} // namespace x86