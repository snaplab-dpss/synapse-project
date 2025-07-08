#include <LibSynapse/Modules/Controller/DataplaneIntegerAllocatorAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> DataplaneIntegerAllocatorAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;

  addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(chain_out);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneIntegerAllocatorAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                           LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate") {
    return {};
  }

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;

  const addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(chain_out);

  if (!ep->get_ctx().check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return {};
  }

  Module *module  = new DataplaneIntegerAllocatorAllocate(node, dchain_addr, index_range);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneIntegerAllocatorAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate") {
    return {};
  }

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;

  addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(chain_out);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
    return {};
  }

  return std::make_unique<DataplaneIntegerAllocatorAllocate>(node, dchain_addr, index_range);
}

} // namespace Controller
} // namespace LibSynapse