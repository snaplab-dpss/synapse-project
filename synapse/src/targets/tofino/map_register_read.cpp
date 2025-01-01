#include "map_register_read.h"

namespace tofino {

namespace {
struct map_register_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  symbol_t map_has_this_key;
  u32 num_entries;

  map_register_data_t(const EP *ep, const Call *map_get) {
    const call_t &call = map_get->get_call();

    obj = expr_addr_to_obj_addr(call.args.at("map").expr);
    key = call.args.at("key").in;
    read_value = call.args.at("value_out").out;

    bool found = get_symbol(map_get->get_locally_generated_symbols(), "map_has_this_key",
                            map_has_this_key);
    ASSERT(found, "Symbol map_has_this_key not found");

    num_entries = ep->get_ctx().get_map_config(obj).capacity;
  }
};

} // namespace

std::optional<spec_impl_t> MapRegisterReadFactory::speculate(const EP *ep,
                                                             const Node *node,
                                                             const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call = map_get->get_call();

  if (call.function_name != "map_get") {
    return std::nullopt;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
      !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister)) {
    return std::nullopt;
  }

  map_register_data_t map_register_data(ep, map_get);

  if (!can_build_or_reuse_map_register(ep, node, map_register_data.obj,
                                       map_register_data.key,
                                       map_register_data.num_entries)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
  new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> MapRegisterReadFactory::process_node(const EP *ep,
                                                         const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call = map_get->get_call();

  if (call.function_name != "map_get") {
    return impls;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
      !ep->get_ctx().can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister)) {
    return impls;
  }

  map_register_data_t map_register_data(ep, map_get);

  MapRegister *map_register =
      build_or_reuse_map_register(ep, node, map_register_data.obj, map_register_data.key,
                                  map_register_data.num_entries);

  if (!map_register) {
    return impls;
  }

  Module *module = new MapRegisterRead(
      node, map_register->id, map_register_data.obj, map_register_data.key,
      map_register_data.read_value, map_register_data.map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, map_objs.map, map_register);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino
