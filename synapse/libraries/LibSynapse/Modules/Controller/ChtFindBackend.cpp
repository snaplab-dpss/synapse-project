#include <LibSynapse/Modules/Controller/ChtFindBackend.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> ChtFindBackendFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cht_find_preferred_available_backend") {
    return {};
  }

  klee::ref<klee::Expr> cht = call.args.at("cht").expr;
  const addr_t cht_addr     = expr_addr_to_obj_addr(cht);

  if (!ctx.can_impl_ds(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ChtFindBackendFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cht_find_preferred_available_backend") {
    return {};
  }

  klee::ref<klee::Expr> cht      = call.args.at("cht").expr;
  klee::ref<klee::Expr> backends = call.args.at("active_backends").expr;
  klee::ref<klee::Expr> hash     = call.args.at("hash").expr;
  klee::ref<klee::Expr> height   = call.args.at("cht_height").expr;
  klee::ref<klee::Expr> capacity = call.args.at("backend_capacity").expr;
  klee::ref<klee::Expr> backend  = call.args.at("chosen_backend").out;

  const addr_t cht_addr      = expr_addr_to_obj_addr(cht);
  const addr_t backends_addr = expr_addr_to_obj_addr(backends);

  const symbol_t backend_found = call_node->get_local_symbol("prefered_backend_found");

  if (!ep->get_ctx().can_impl_ds(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return {};
  }

  Module *module  = new ChtFindBackend(type, node, cht_addr, backends_addr, hash, height, capacity, backend, backend_found);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(cht_addr, DSImpl::Controller_ConsistentHashTable);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ChtFindBackendFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cht_find_preferred_available_backend") {
    return {};
  }

  klee::ref<klee::Expr> cht      = call.args.at("cht").expr;
  klee::ref<klee::Expr> backends = call.args.at("active_backends").expr;
  klee::ref<klee::Expr> hash     = call.args.at("hash").expr;
  klee::ref<klee::Expr> height   = call.args.at("cht_height").expr;
  klee::ref<klee::Expr> capacity = call.args.at("backend_capacity").expr;
  klee::ref<klee::Expr> backend  = call.args.at("chosen_backend").out;

  const symbol_t backend_found = call_node->get_local_symbol("prefered_backend_found");

  const addr_t cht_addr      = expr_addr_to_obj_addr(cht);
  const addr_t backends_addr = expr_addr_to_obj_addr(backends);

  if (!ctx.check_ds_impl(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return {};
  }

  return std::make_unique<ChtFindBackend>(type, node, cht_addr, backends_addr, hash, height, capacity, backend, backend_found);
}

} // namespace Controller
} // namespace LibSynapse