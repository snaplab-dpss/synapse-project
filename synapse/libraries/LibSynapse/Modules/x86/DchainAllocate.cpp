#include <LibSynapse/Modules/x86/DchainAllocate.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>
#include <klee/util/Ref.h>

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
  if (call.function_name != "dchain_allocate") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> DchainAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> chain_out = call.args.at("chain_out").out;
  const addr_t dchain_addr        = expr_addr_to_obj_addr(chain_out);

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::x86_DoubleChain)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;
  symbol_t success                  = call_node->get_local_symbol("is_dchain_allocated");
  const addr_t dchain_addr          = expr_addr_to_obj_addr(chain_out);

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::x86_DoubleChain)) {
    return {};
  }

  Module *module  = new DchainAllocate(get_type().instance_id, node, index_range, chain_out, success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::x86_DoubleChain);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DchainAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;
  symbol_t success                  = call_node->get_local_symbol("is_dchain_allocated");

  const addr_t dchain_addr = expr_addr_to_obj_addr(chain_out);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::x86_DoubleChain)) {
    return {};
  }

  return std::make_unique<DchainAllocate>(get_type().instance_id, node, index_range, chain_out, success);
}

} // namespace x86
} // namespace LibSynapse
