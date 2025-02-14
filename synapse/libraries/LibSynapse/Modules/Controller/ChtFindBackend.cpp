#include <LibSynapse/Modules/Controller/ChtFindBackend.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> ChtFindBackendFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cht_find_preferred_available_backend") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> cht = call.args.at("cht").expr;
  addr_t cht_addr           = LibCore::expr_addr_to_obj_addr(cht);

  if (!ctx.can_impl_ds(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ChtFindBackendFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                        LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cht_find_preferred_available_backend") {
    return impls;
  }

  klee::ref<klee::Expr> cht      = call.args.at("cht").expr;
  klee::ref<klee::Expr> backends = call.args.at("active_backends").expr;
  klee::ref<klee::Expr> hash     = call.args.at("hash").expr;
  klee::ref<klee::Expr> height   = call.args.at("cht_height").expr;
  klee::ref<klee::Expr> capacity = call.args.at("backend_capacity").expr;
  klee::ref<klee::Expr> backend  = call.args.at("chosen_backend").out;

  addr_t cht_addr      = LibCore::expr_addr_to_obj_addr(cht);
  addr_t backends_addr = LibCore::expr_addr_to_obj_addr(backends);

  LibCore::symbol_t backend_found = call_node->get_local_symbol("prefered_backend_found");

  if (!ep->get_ctx().can_impl_ds(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return impls;
  }

  Module *module  = new ChtFindBackend(node, cht_addr, backends_addr, hash, height, capacity, backend, backend_found);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(cht_addr, DSImpl::Controller_ConsistentHashTable);

  return impls;
}

std::unique_ptr<Module> ChtFindBackendFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cht_find_preferred_available_backend") {
    return {};
  }

  klee::ref<klee::Expr> cht       = call.args.at("cht").expr;
  klee::ref<klee::Expr> backends  = call.args.at("active_backends").expr;
  klee::ref<klee::Expr> hash      = call.args.at("hash").expr;
  klee::ref<klee::Expr> height    = call.args.at("cht_height").expr;
  klee::ref<klee::Expr> capacity  = call.args.at("backend_capacity").expr;
  klee::ref<klee::Expr> backend   = call.args.at("chosen_backend").out;
  LibCore::symbol_t backend_found = call_node->get_local_symbol("prefered_backend_found");

  addr_t cht_addr      = LibCore::expr_addr_to_obj_addr(cht);
  addr_t backends_addr = LibCore::expr_addr_to_obj_addr(backends);

  if (!ctx.check_ds_impl(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return {};
  }

  return std::make_unique<ChtFindBackend>(node, cht_addr, backends_addr, hash, height, capacity, backend, backend_found);
}

} // namespace Controller
} // namespace LibSynapse