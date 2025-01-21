#include <LibSynapse/Modules/x86/MapGet.h>
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

  if (call.function_name != "map_get") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> MapGetFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  addr_t map_addr                     = LibCore::expr_addr_to_obj_addr(map_addr_expr);

  if (!ep->get_ctx().can_impl_ds(map_addr, DSImpl::x86_Map)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> MapGetFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr_expr = call.args.at("key").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> success       = call.ret;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  LibCore::symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");
  addr_t map_addr                    = LibCore::expr_addr_to_obj_addr(map_addr_expr);
  addr_t key_addr                    = LibCore::expr_addr_to_obj_addr(key_addr_expr);

  if (!ep->get_ctx().can_impl_ds(map_addr, DSImpl::x86_Map)) {
    return impls;
  }

  Module *module  = new MapGet(node, map_addr, key_addr, key, value_out, success, map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(map_addr, DSImpl::x86_Map);

  return impls;
}

} // namespace x86
} // namespace LibSynapse