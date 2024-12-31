#include "int_alloc_allocate.h"

namespace tofino {

std::optional<spec_impl_t>
IntegerAllocatorAllocateGenerator::speculate(const EP *ep, const Node *node,
                                             const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t>
IntegerAllocatorAllocateGenerator::process_node(const EP *ep,
                                                const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return impls;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out = call.args.at("index_out").out;

  addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ep->get_ctx().can_impl_ds(dchain_addr,
                                 DSImpl::Tofino_IntegerAllocator)) {
    return impls;
  }

  symbols_t symbols = call_node->get_locally_generated_symbols();
  symbol_t out_of_space;
  bool found = get_symbol(symbols, "out_of_space", out_of_space);
  ASSERT(found, "Symbol out_of_space not found");

  // TODO: implement the actual data structure.

  Module *module = new IntegerAllocatorAllocate(node, dchain_addr, time,
                                                index_out, out_of_space);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr,
                                         DSImpl::Tofino_IntegerAllocator);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino
