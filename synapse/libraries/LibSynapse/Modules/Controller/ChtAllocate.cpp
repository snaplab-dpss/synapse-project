#include <LibSynapse/Modules/Controller/ChtAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> ChtAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cht_fill_cht") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> cht = call.args.at("cht").expr;
  addr_t cht_addr           = LibCore::expr_addr_to_obj_addr(cht);

  if (!ctx.can_impl_ds(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ChtAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cht_fill_cht") {
    return impls;
  }

  klee::ref<klee::Expr> cht        = call.args.at("cht").expr;
  klee::ref<klee::Expr> cht_height = call.args.at("cht_height").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("backend_capacity").expr;

  addr_t cht_addr = LibCore::expr_addr_to_obj_addr(cht);

  if (!ep->get_ctx().can_impl_ds(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return impls;
  }

  Module *module  = new ChtAllocate(node, cht_addr, cht_height, capacity);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(cht_addr, DSImpl::Controller_ConsistentHashTable);

  return impls;
}

std::unique_ptr<Module> ChtAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cht_fill_cht") {
    return {};
  }

  klee::ref<klee::Expr> cht        = call.args.at("cht").expr;
  klee::ref<klee::Expr> cht_height = call.args.at("cht_height").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("backend_capacity").expr;

  addr_t cht_addr = LibCore::expr_addr_to_obj_addr(cht);

  if (!ctx.check_ds_impl(cht_addr, DSImpl::Controller_ConsistentHashTable)) {
    return {};
  }

  return std::make_unique<ChtAllocate>(node, cht_addr, cht_height, capacity);
}

} // namespace Controller
} // namespace LibSynapse