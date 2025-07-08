#include <LibSynapse/Modules/Controller/DchainAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> DchainAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> chain_out = call.args.at("chain_out").out;
  addr_t dchain_addr              = LibCore::expr_addr_to_obj_addr(chain_out);

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate") {
    return {};
  }

  klee::ref<klee::Expr> chain_out = call.args.at("chain_out").out;
  const addr_t dchain_addr        = LibCore::expr_addr_to_obj_addr(chain_out);

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return {};
  }

  Module *module  = new DchainAllocate(node, dchain_addr);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::Controller_DoubleChain);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DchainAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate") {
    return {};
  }

  klee::ref<klee::Expr> chain_out = call.args.at("chain_out").out;
  addr_t dchain_addr              = LibCore::expr_addr_to_obj_addr(chain_out);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Controller_DoubleChain)) {
    return {};
  }

  return std::make_unique<DchainAllocate>(node, dchain_addr);
}

} // namespace Controller
} // namespace LibSynapse