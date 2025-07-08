#include <LibSynapse/Modules/Tofino/IntegerAllocatorAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

std::optional<spec_impl_t> IntegerAllocatorAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
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

std::vector<impl_t> IntegerAllocatorAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                  LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return {};
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time             = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;

  const addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return {};
  }

  const LibCore::symbol_t not_out_of_space = call_node->get_local_symbol("not_out_of_space");

  // TODO: implement the actual data structure.

  Module *module  = new IntegerAllocatorAllocate(node, dchain_addr, time, index_out, not_out_of_space);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> IntegerAllocatorAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return {};
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time             = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;
  LibCore::symbol_t not_out_of_space     = call_node->get_local_symbol("not_out_of_space");

  addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return {};
  }

  return std::make_unique<IntegerAllocatorAllocate>(node, dchain_addr, time, index_out, not_out_of_space);
}

} // namespace Tofino
} // namespace LibSynapse