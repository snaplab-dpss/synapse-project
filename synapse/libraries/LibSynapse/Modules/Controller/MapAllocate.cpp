#include <LibSynapse/Modules/Controller/MapAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> MapAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_allocate") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  addr_t map_addr                     = LibCore::expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.can_impl_ds(map_addr, DSImpl::Controller_Map)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> MapAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_allocate") {
    return impls;
  }

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out  = call.args.at("map_out").out;

  addr_t map_addr = LibCore::expr_addr_to_obj_addr(map_out);

  if (!ep->get_ctx().can_impl_ds(map_addr, DSImpl::Controller_Map)) {
    return impls;
  }

  Module *module  = new MapAllocate(node, map_addr, capacity, key_size);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(map_addr, DSImpl::Controller_Map);

  return impls;
}

std::unique_ptr<Module> MapAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out  = call.args.at("map_out").out;

  addr_t map_addr = LibCore::expr_addr_to_obj_addr(map_out);

  if (!ctx.check_ds_impl(map_addr, DSImpl::Controller_Map)) {
    return {};
  }

  return std::make_unique<MapAllocate>(node, map_addr, capacity, key_size);
}

} // namespace Controller
} // namespace LibSynapse