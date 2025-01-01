#include "fcfs_cached_table_read.h"

namespace tofino {

namespace {
struct fcfs_cached_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  symbol_t map_has_this_key;
  u32 num_entries;

  fcfs_cached_table_data_t(const EP *ep, const Call *map_get) {
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

EP *concretize_cached_table_read(const EP *ep, const Node *node,
                                 const map_coalescing_objs_t &map_objs,
                                 const fcfs_cached_table_data_t &cached_table_data,
                                 u32 cache_capacity) {
  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
      ep, node, cached_table_data.obj, cached_table_data.key,
      cached_table_data.num_entries, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  Module *module = new FCFSCachedTableRead(
      node, cached_table->id, cached_table_data.obj, cached_table_data.key,
      cached_table_data.read_value, cached_table_data.map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, map_objs.map, cached_table);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return new_ep;
}
} // namespace

std::optional<spec_impl_t>
FCFSCachedTableReadFactory::speculate(const EP *ep, const Node *node,
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

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return std::nullopt;
  }

  fcfs_cached_table_data_t cached_table_data(ep, map_get);

  FCFSCachedTable *fcfs_cached_table =
      get_fcfs_cached_table(ep, node, cached_table_data.obj);
  if (!fcfs_cached_table) {
    return std::nullopt;
  }

  Context new_ctx = ctx;

  new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  spec_impl_t spec_impl(
      decide(ep, node, {{CACHE_SIZE_PARAM, fcfs_cached_table->cache_capacity}}), new_ctx);

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableReadFactory::process_node(const EP *ep,
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

  if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return impls;
  }

  fcfs_cached_table_data_t cached_table_data(ep, map_get);

  std::vector<u32> allowed_cache_capacities =
      enum_fcfs_cache_cap(cached_table_data.num_entries);

  for (u32 cache_capacity : allowed_cache_capacities) {
    EP *new_ep = concretize_cached_table_read(ep, node, map_objs, cached_table_data,
                                              cache_capacity);

    if (new_ep) {
      impl_t impl = implement(ep, node, new_ep, {{CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(impl);
    }
  }

  return impls;
}

} // namespace tofino
