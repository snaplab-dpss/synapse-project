#include <LibSynapse/Modules/Controller/DataplaneIntegerAllocatorAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> DataplaneIntegerAllocatorAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate") {
    return {};
  }

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;

  const addr_t dchain_addr = expr_addr_to_obj_addr(chain_out);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneIntegerAllocatorAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate") {
    return {};
  }

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;

  const addr_t dchain_addr = expr_addr_to_obj_addr(chain_out);

  if (!ep->get_ctx().check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return {};
  }

  Module *module  = new DataplaneIntegerAllocatorAllocate(node, dchain_addr, index_range);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneIntegerAllocatorAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate") {
    return {};
  }

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;

  const addr_t dchain_addr = expr_addr_to_obj_addr(chain_out);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return {};
  }

  return std::make_unique<DataplaneIntegerAllocatorAllocate>(node, dchain_addr, index_range);
}

} // namespace Controller
} // namespace LibSynapse