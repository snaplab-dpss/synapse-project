#include <LibSynapse/Modules/x86/DchainAllocateNewIndex.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> DchainAllocateNewIndexFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  const addr_t dchain_addr               = expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::x86_DoubleChain)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainAllocateNewIndexFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time             = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;

  const addr_t dchain_addr        = expr_addr_to_obj_addr(dchain_addr_expr);
  const symbol_t not_out_of_space = call_node->get_local_symbol("not_out_of_space");

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::x86_DoubleChain)) {
    return {};
  }

  Module *module  = new DchainAllocateNewIndex(get_type().instance_id, node, dchain_addr, time, index_out, not_out_of_space);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::x86_DoubleChain);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DchainAllocateNewIndexFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> time             = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;

  const addr_t dchain_addr        = expr_addr_to_obj_addr(dchain_addr_expr);
  const symbol_t not_out_of_space = call_node->get_local_symbol("not_out_of_space");

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::x86_DoubleChain)) {
    return {};
  }

  return std::make_unique<DchainAllocateNewIndex>(get_type().instance_id, node, dchain_addr, time, index_out, not_out_of_space);
}

} // namespace x86
} // namespace LibSynapse