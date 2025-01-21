#include <LibSynapse/Modules/Tofino/Ignore.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

bool can_ignore_vector_register_op(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();

  // Write operations are made on vector_return, not vector_borrow.
  if (call.function_name == "vector_borrow" && call_node->is_vector_write()) {
    return true;
  }

  // Read operations are made on vector_borrow, not vector_return.
  if (call.function_name == "vector_return" && call_node->is_vector_read()) {
    return true;
  }

  if (call_node->is_vector_borrow_value_ignored()) {
    return true;
  }

  return false;
}

bool can_ignore_fcfs_cached_table_op(const Context &ctx, const LibBDD::call_t &call) {
  if (call.function_name != "dchain_free_index" && call.function_name != "dchain_allocate_new_index") {
    return false;
  }

  klee::ref<klee::Expr> dchain = call.args.at("chain").expr;
  addr_t dchain_addr           = LibCore::expr_addr_to_obj_addr(dchain);

  if (!ctx.check_ds_impl(dchain_addr, DSImpl::Tofino_FCFSCachedTable)) {
    return false;
  }

  return true;
}

// We can ignore dchain_rejuvenate_index if the dchain is only used for
// linking a map with vectors. It doesn't even matter if the data structures
// are coalesced or not, we can freely ignore it regardless.
bool can_ignore_dchain_rejuvenation(const Context &ctx, const LibBDD::call_t &call) {
  if (call.function_name != "dchain_rejuvenate_index") {
    return false;
  }

  klee::ref<klee::Expr> chain = call.args.at("chain").expr;
  addr_t chain_addr           = LibCore::expr_addr_to_obj_addr(chain);

  // These are the data structures that can perform rejuvenations.
  if (!ctx.check_ds_impl(chain_addr, DSImpl::Tofino_Table) && !ctx.check_ds_impl(chain_addr, DSImpl::Tofino_FCFSCachedTable) &&
      !ctx.check_ds_impl(chain_addr, DSImpl::Tofino_HeavyHitterTable)) {
    return false;
  }

  return true;
}

bool should_ignore(const EP *ep, const Context &ctx, const LibBDD::Node *node) {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return false;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  const std::unordered_set<std::string> functions_to_always_ignore{
      "expire_items_single_map", "expire_items_single_map_iteratively", "tb_expire", "nf_set_rte_ipv4_udptcp_checksum",
      "cms_periodic_cleanup",
  };

  if (functions_to_always_ignore.find(call.function_name) != functions_to_always_ignore.end()) {
    return true;
  }

  if (can_ignore_fcfs_cached_table_op(ctx, call)) {
    return true;
  }

  if (can_ignore_dchain_rejuvenation(ctx, call)) {
    return true;
  }

  if (can_ignore_vector_register_op(call_node)) {
    return true;
  }

  return false;
}
} // namespace

std::optional<spec_impl_t> IgnoreFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (!should_ignore(ep, ctx, node)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> IgnoreFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!should_ignore(ep, ep->get_ctx(), node)) {
    return impls;
  }

  Module *module  = new Ignore(node);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace Tofino
} // namespace LibSynapse