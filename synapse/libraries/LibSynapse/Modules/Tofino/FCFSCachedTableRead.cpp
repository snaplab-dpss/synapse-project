#include <LibSynapse/Modules/Tofino/FCFSCachedTableRead.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct fcfs_cached_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  symbol_t map_has_this_key;
  u32 capacity;
  map_coalescing_objs_t map_objs;
};

std::optional<fcfs_cached_table_data_t> build_fcfs_cached_table_data(const BDD *bdd, const Context &ctx, const Call *map_get) {
  const call_t &call = map_get->get_call();

  fcfs_cached_table_data_t data;
  data.obj              = expr_addr_to_obj_addr(call.args.at("map").expr);
  data.original_key     = call.args.at("key").in;
  data.keys             = Table::build_keys(data.original_key, ctx.get_expr_structs());
  data.map_has_this_key = map_get->get_local_symbol("map_has_this_key");
  data.capacity         = ctx.get_map_config(data.obj).capacity;

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  data.map_objs = map_objs.value();

  std::vector<BDD::vector_values_t> values;
  if (!data.map_objs.vectors.empty()) {
    values = bdd->get_vector_values_from_map_op(map_get);
  }

  if (values.size() != 0) {
    return {};
  }

  return data;
}

std::unique_ptr<EP> concretize_cached_table_read(const EP *ep, const BDDNode *node, const fcfs_cached_table_data_t &fcfs_cached_table_data,
                                                 u32 cache_capacity) {
  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
      ep, node, fcfs_cached_table_data.obj, fcfs_cached_table_data.original_key, fcfs_cached_table_data.capacity, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  Module *module  = new FCFSCachedTableRead(node, cached_table->id, cached_table->tables.back().id, fcfs_cached_table_data.obj,
                                            fcfs_cached_table_data.original_key, fcfs_cached_table_data.keys, fcfs_cached_table_data.map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(fcfs_cached_table_data.map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(fcfs_cached_table_data.map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, fcfs_cached_table_data.map_objs.map, cached_table);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return new_ep;
}

} // namespace

std::optional<spec_impl_t> FCFSCachedTableReadFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const std::optional<fcfs_cached_table_data_t> fcfs_cached_table_data = build_fcfs_cached_table_data(ep->get_bdd(), ep->get_ctx(), map_get);
  if (!fcfs_cached_table_data.has_value()) {
    return {};
  }

  if (!ctx.can_impl_ds(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.can_impl_ds(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (!ep->get_bdd()->is_dchain_used_exclusively_for_linking_maps_with_vectors(fcfs_cached_table_data->map_objs.dchain)) {
    return {};
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_fcfs_cached_table_id(fcfs_cached_table_data->map_objs.map))) {
      return {};
    }
  }

  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(fcfs_cached_table_data->capacity);

  // Let's optimistically pick the largest cache capacity that we can build or reuse.
  u32 cache_capacity;
  for (auto rev_it = allowed_cache_capacities.rbegin(); rev_it != allowed_cache_capacities.rend(); rev_it++) {
    if (can_build_or_reuse_fcfs_cached_table(ep, node, fcfs_cached_table_data->obj, fcfs_cached_table_data->original_key,
                                             fcfs_cached_table_data->capacity, *rev_it)) {
      cache_capacity = *rev_it;
      break;
    }
  }

  Context new_ctx = ctx;

  new_ctx.save_ds_impl(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}}), new_ctx);

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableReadFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const std::optional<fcfs_cached_table_data_t> fcfs_cached_table_data = build_fcfs_cached_table_data(ep->get_bdd(), ep->get_ctx(), map_get);
  if (!fcfs_cached_table_data.has_value()) {
    return {};
  }

  if (!ep->get_bdd()->is_dchain_used_exclusively_for_linking_maps_with_vectors(fcfs_cached_table_data->map_objs.dchain)) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_fcfs_cached_table_id(fcfs_cached_table_data->map_objs.map))) {
      return {};
    }
  }

  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(fcfs_cached_table_data->capacity);

  std::vector<impl_t> impls;
  for (u32 cache_capacity : allowed_cache_capacities) {
    std::unique_ptr<EP> new_ep = concretize_cached_table_read(ep, node, fcfs_cached_table_data.value(), cache_capacity);
    if (new_ep) {
      impl_t impl = implement(ep, node, std::move(new_ep), {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(std::move(impl));
    }
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableReadFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const std::optional<fcfs_cached_table_data_t> fcfs_cached_table_data = build_fcfs_cached_table_data(bdd, ctx, map_get);
  if (!fcfs_cached_table_data.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(fcfs_cached_table_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.check_ds_impl(fcfs_cached_table_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(fcfs_cached_table_data->map_objs.map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedTable *fcfs_cached_table = dynamic_cast<const FCFSCachedTable *>(*ds.begin());

  return std::make_unique<FCFSCachedTableRead>(node, fcfs_cached_table->id, fcfs_cached_table->tables.back().id, fcfs_cached_table_data->obj,
                                               fcfs_cached_table_data->original_key, fcfs_cached_table_data->keys,
                                               fcfs_cached_table_data->map_has_this_key);
}

} // namespace Tofino
} // namespace LibSynapse
