#include "int_alloc_free_index.h"

namespace synapse {
namespace controller {

std::optional<spec_impl_t>
IntegerAllocatorFreeIndexFactory::speculate(const EP *ep, const Node *node,
                                            const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "dchain_free_index") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t>
IntegerAllocatorFreeIndexFactory::process_node(const EP *ep, const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "dchain_free_index") {
    return impls;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;

  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ep->get_ctx().check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return impls;
  }

  Module *module = new IntegerAllocatorFreeIndex(node, dchain_addr, index);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace controller
} // namespace synapse