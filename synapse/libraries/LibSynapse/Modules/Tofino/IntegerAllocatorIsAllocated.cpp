#include <LibSynapse/Modules/Tofino/IntegerAllocatorIsAllocated.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

std::optional<spec_impl_t> IntegerAllocatorIsAllocatedFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  addr_t dchain_addr                     = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> IntegerAllocatorIsAllocatedFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                     LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return impls;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;

  addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return impls;
  }

  LibCore::symbol_t is_allocated = call_node->get_local_symbol("dchain_is_index_allocated");
  Module *module                 = new IntegerAllocatorIsAllocated(node, dchain_addr, index, is_allocated);
  EPNode *ep_node                = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace Tofino
} // namespace LibSynapse