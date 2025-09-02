#include <LibSynapse/Modules/Controller/DchainAllocateNewIndex.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> DchainAllocateNewIndexFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return {};
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  const addr_t dchain_addr               = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainAllocateNewIndexFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return {};
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time             = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;

  const addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return {};
  }

  Module *module;
  if (call_node->has_local_symbol("not_out_of_space")) {
    symbol_t not_out_of_space = call_node->get_local_symbol("not_out_of_space");
    module                    = new DchainAllocateNewIndex(ep->get_placement(node->get_id()), node, dchain_addr, time, index_out, not_out_of_space);
  } else {
    module = new DchainAllocateNewIndex(ep->get_placement(node->get_id()), node, dchain_addr, time, index_out);
  }

  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::Controller_DoubleChain);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DchainAllocateNewIndexFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return {};
  }

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time             = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;

  const addr_t dchain_addr        = expr_addr_to_obj_addr(dchain_addr_expr);
  const symbol_t not_out_of_space = call_node->get_local_symbol("not_out_of_space");

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return {};
  }

  return std::make_unique<DchainAllocateNewIndex>(get_type().instance_id, node, dchain_addr, time, index_out, not_out_of_space);
}

} // namespace Controller
} // namespace LibSynapse