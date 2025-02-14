#include <LibSynapse/Modules/Controller/DchainAllocateNewIndex.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> DchainAllocateNewIndexFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
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

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainAllocateNewIndexFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return impls;
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time             = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;

  addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return impls;
  }

  Module *module;
  if (call_node->has_local_symbol("not_out_of_space")) {
    LibCore::symbol_t not_out_of_space = call_node->get_local_symbol("not_out_of_space");
    module                             = new DchainAllocateNewIndex(node, dchain_addr, time, index_out, not_out_of_space);
  } else {
    module = new DchainAllocateNewIndex(node, dchain_addr, time, index_out);
  }

  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::Controller_DoubleChain);

  return impls;
}

std::unique_ptr<Module> DchainAllocateNewIndexFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
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

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return {};
  }

  return std::make_unique<DchainAllocateNewIndex>(node, dchain_addr, time, index_out, not_out_of_space);
}

} // namespace Controller
} // namespace LibSynapse