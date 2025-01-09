#include "cht_find_backend.h"

namespace synapse {
namespace x86 {
namespace {
bool bdd_node_match_pattern(const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "cht_find_preferred_available_backend") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ChtFindBackendFactory::speculate(const EP *ep, const Node *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> cht = call.args.at("cht").expr;
  addr_t cht_addr = expr_addr_to_obj_addr(cht);

  if (!ctx.can_impl_ds(cht_addr, DSImpl::x86_ConsistentHashTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ChtFindBackendFactory::process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> cht = call.args.at("cht").expr;
  klee::ref<klee::Expr> backends = call.args.at("active_backends").expr;
  klee::ref<klee::Expr> hash = call.args.at("hash").expr;
  klee::ref<klee::Expr> height = call.args.at("cht_height").expr;
  klee::ref<klee::Expr> capacity = call.args.at("backend_capacity").expr;
  klee::ref<klee::Expr> backend = call.args.at("chosen_backend").out;

  addr_t cht_addr = expr_addr_to_obj_addr(cht);
  addr_t backends_addr = expr_addr_to_obj_addr(backends);

  if (!ep->get_ctx().can_impl_ds(cht_addr, DSImpl::x86_ConsistentHashTable)) {
    return impls;
  }

  symbol_t backend_found = call_node->get_local_symbol("prefered_backend_found");
  Module *module = new ChtFindBackend(node, cht_addr, backends_addr, hash, height, capacity, backend, backend_found);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(cht_addr, DSImpl::x86_ConsistentHashTable);

  return impls;
}

} // namespace x86
} // namespace synapse