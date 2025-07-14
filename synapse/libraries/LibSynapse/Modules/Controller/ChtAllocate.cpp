#include <LibSynapse/Modules/Controller/ChtAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> ChtAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cht_fill_cht") {
    return {};
  }

  klee::ref<klee::Expr> cht = call.args.at("cht").expr;
  const addr_t cht_addr     = expr_addr_to_obj_addr(cht);

  if (!ctx.can_impl_ds(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ChtAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cht_fill_cht") {
    return {};
  }

  klee::ref<klee::Expr> cht        = call.args.at("cht").expr;
  klee::ref<klee::Expr> cht_height = call.args.at("cht_height").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("backend_capacity").expr;

  const addr_t cht_addr = expr_addr_to_obj_addr(cht);

  if (!ep->get_ctx().can_impl_ds(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return {};
  }

  Module *module  = new ChtAllocate(node, cht_addr, cht_height, capacity);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(cht_addr, DSImpl::Controller_ConsistentHashTable);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> ChtAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cht_fill_cht") {
    return {};
  }

  klee::ref<klee::Expr> cht        = call.args.at("cht").expr;
  klee::ref<klee::Expr> cht_height = call.args.at("cht_height").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("backend_capacity").expr;

  const addr_t cht_addr = expr_addr_to_obj_addr(cht);

  if (!ctx.check_ds_impl(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return {};
  }

  return std::make_unique<ChtAllocate>(node, cht_addr, cht_height, capacity);
}

} // namespace Controller
} // namespace LibSynapse