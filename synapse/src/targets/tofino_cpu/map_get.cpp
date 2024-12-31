#include "map_get.h"

namespace tofino_cpu {

std::optional<spec_impl_t>
MapGetGenerator::speculate(const EP *ep, const Node *node,
                           const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "map_get") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.can_impl_ds(map_addr, DSImpl::TofinoCPU_Map)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> MapGetGenerator::process_node(const EP *ep,
                                                  const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "map_get") {
    return impls;
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key_addr_expr = call.args.at("key").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> success = call.ret;
  klee::ref<klee::Expr> value_out = call.args.at("value_out").out;

  symbols_t symbols = call_node->get_locally_generated_symbols();

  symbol_t map_has_this_key;
  bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
  ASSERT(found, "Symbol map_has_this_key not found");

  addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);
  addr_t key_addr = expr_addr_to_obj_addr(key_addr_expr);

  if (!ep->get_ctx().can_impl_ds(map_addr, DSImpl::TofinoCPU_Map)) {
    return impls;
  }

  Module *module = new MapGet(node, map_addr, key_addr, key, value_out, success,
                              map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(map_addr, DSImpl::TofinoCPU_Map);

  return impls;
}

} // namespace tofino_cpu
