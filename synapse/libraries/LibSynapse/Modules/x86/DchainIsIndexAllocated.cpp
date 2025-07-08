#include <LibSynapse/Modules/x86/DchainIsIndexAllocated.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

namespace {
bool bdd_node_match_pattern(const LibBDD::Node *node) {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return false;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> DchainIsIndexAllocatedFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  addr_t dchain_addr                     = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.can_impl_ds(dchain_addr, DSImpl::x86_DoubleChain)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainIsIndexAllocatedFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                LibCore::SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;

  const addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::x86_DoubleChain)) {
    return {};
  }

  LibCore::symbol_t is_allocated = call_node->get_local_symbol("is_allocated");
  Module *module                 = new DchainIsIndexAllocated(node, dchain_addr, index, is_allocated);
  EPNode *ep_node                = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(dchain_addr, DSImpl::x86_DoubleChain);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DchainIsIndexAllocatedFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;

  addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::x86_DoubleChain)) {
    return {};
  }

  return std::make_unique<DchainIsIndexAllocated>(node, dchain_addr, index, call_node->get_local_symbol("is_allocated"));
}

} // namespace x86
} // namespace LibSynapse