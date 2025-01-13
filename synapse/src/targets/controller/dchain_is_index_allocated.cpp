#include "dchain_is_index_allocated.h"

namespace synapse {
namespace ctrl {

std::optional<spec_impl_t> DchainIsIndexAllocatedFactory::speculate(const EP *ep, const Node *node,
                                                                    const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  addr_t dchain_addr                     = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainIsIndexAllocatedFactory::process_node(const EP *ep, const Node *node,
                                                                SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return impls;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;

  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return impls;
  }

  symbol_t is_allocated = call_node->get_local_symbol("dchain_is_index_allocated");

  Module *module  = new DchainIsIndexAllocated(node, dchain_addr, index, is_allocated);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::Controller_DoubleChain);

  return impls;
}

} // namespace ctrl
} // namespace synapse