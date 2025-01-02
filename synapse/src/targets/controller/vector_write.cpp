#include "vector_write.h"

namespace synapse {
namespace controller {

std::optional<spec_impl_t> VectorWriteFactory::speculate(const EP *ep, const Node *node,
                                                         const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "vector_return") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::Controller_Vector)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> VectorWriteFactory::process_node(const EP *ep,
                                                     const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "vector_return") {
    return impls;
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr = call.args.at("value").expr;
  klee::ref<klee::Expr> value = call.args.at("value").in;

  addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);
  addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

  // We don't need to place the vector, we will never get a vector_return
  // before a vector_borrow.
  if (!ep->get_ctx().check_ds_impl(vector_addr, DSImpl::Controller_Vector)) {
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

  return impls;
}

} // namespace controller
} // namespace synapse