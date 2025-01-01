#include "dchain_allocate_new_index.h"

namespace tofino_cpu {

std::optional<spec_impl_t>
DchainAllocateNewIndexFactory::speculate(const EP *ep, const Node *node,
                                         const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::TofinoCPU_DoubleChain)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainAllocateNewIndexFactory::process_node(const EP *ep,
                                                                const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return impls;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out = call.args.at("index_out").out;

  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::TofinoCPU_DoubleChain)) {
    return impls;
  }

  symbols_t symbols = call_node->get_locally_generated_symbols();
  symbol_t out_of_space;
  bool found = get_symbol(symbols, "out_of_space", out_of_space);

  Module *module;
  if (found) {
    module = new DchainAllocateNewIndex(node, dchain_addr, time, index_out, out_of_space);
  } else {
    module = new DchainAllocateNewIndex(node, dchain_addr, time, index_out);
  }

  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::TofinoCPU_DoubleChain);

  return impls;
}

} // namespace tofino_cpu
