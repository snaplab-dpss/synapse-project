#include "vector_read.h"

namespace synapse {
namespace controller {

std::optional<spec_impl_t> VectorReadFactory::speculate(const EP *ep, const Node *node,
                                                        const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::Controller_Vector)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> VectorReadFactory::process_node(const EP *ep,
                                                    const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return impls;
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr = call.args.at("val_out").out;
  klee::ref<klee::Expr> value = call.extra_vars.at("borrowed_cell").second;

  addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);
  addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

  if (!ep->get_ctx().can_impl_ds(vector_addr, DSImpl::Controller_Vector)) {
    return impls;
  }

  Module *module = new VectorRead(node, vector_addr, index, value_addr, value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(vector_addr, DSImpl::Controller_Vector);

  return impls;
}

} // namespace controller
} // namespace synapse