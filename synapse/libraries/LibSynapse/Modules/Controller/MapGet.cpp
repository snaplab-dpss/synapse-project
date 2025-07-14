#include <LibSynapse/Modules/Controller/MapGet.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> MapGetFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  const addr_t map_addr               = expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.can_impl_ds(map_addr, DSImpl::Controller_Map)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> MapGetFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr_expr = call.args.at("key").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> success       = call.ret;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");

  const addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);
  const addr_t key_addr = expr_addr_to_obj_addr(key_addr_expr);

  if (!ep->get_ctx().can_impl_ds(map_addr, DSImpl::Controller_Map)) {
    return {};
  }

  Module *module  = new MapGet(node, map_addr, key_addr, key, value_out, success, map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(map_addr, DSImpl::Controller_Map);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> MapGetFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr_expr = call.args.at("key").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> success       = call.ret;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  const addr_t map_addr           = expr_addr_to_obj_addr(map_addr_expr);
  const addr_t key_addr           = expr_addr_to_obj_addr(key_addr_expr);
  const symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");

  if (!ctx.check_ds_impl(map_addr, DSImpl::Controller_Map)) {
    return {};
  }

  return std::make_unique<MapGet>(node, map_addr, key_addr, key, value_out, success, map_has_this_key);
}

} // namespace Controller
} // namespace LibSynapse