#include <LibSynapse/Modules/Tofino/FCFSCachedSetRead.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct fcfs_cs_data_t {
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  symbol_t map_has_this_key;
  u32 capacity;
  map_coalescing_objs_t map_objs;
};

std::optional<fcfs_cs_data_t> build_fcfs_cs_data(const BDD *bdd, const Context &ctx, const Call *map_get) {
  const call_t &call = map_get->get_call();

  fcfs_cs_data_t data;
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

  return data;
}

std::unique_ptr<EP> concretize(const EP *ep, const BDDNode *node, const fcfs_cs_data_t &fcfs_cs_data, u32 cache_capacity) {
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  FCFSCachedSet *fcfs_cs = TofinoModuleFactory::build_or_reuse_fcfs_cs(new_ep.get(), node, fcfs_cs_data.obj, fcfs_cs_data.original_key,
                                                                       fcfs_cs_data.capacity, cache_capacity);
  if (!fcfs_cs) {
    return nullptr;
  }

  Module *module =
      new FCFSCachedSetRead(node, fcfs_cs->id, fcfs_cs_data.obj, fcfs_cs_data.original_key, fcfs_cs_data.keys, fcfs_cs_data.map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(fcfs_cs_data.map_objs.map, DSImpl::Tofino_FCFSCachedSet);
  ctx.save_ds_impl(fcfs_cs_data.map_objs.dchain, DSImpl::Tofino_FCFSCachedSet);

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, fcfs_cs_data.map_objs.map, fcfs_cs);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return new_ep;
}

} // namespace

std::optional<spec_impl_t> FCFSCachedSetReadFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const std::optional<fcfs_cs_data_t> fcfs_cs_data = build_fcfs_cs_data(ep->get_bdd(), ep->get_ctx(), map_get);
  if (!fcfs_cs_data.has_value()) {
    return {};
  }

  if (!fcfs_cs_data->map_objs.vectors.empty()) {
    return {};
  }

  if (!ctx.is_dchain_used_exclusively_for_linking_maps_with_vectors(fcfs_cs_data->map_objs.dchain)) {
    return {};
  }

  if (!ctx.can_impl_ds(fcfs_cs_data->map_objs.map, DSImpl::Tofino_FCFSCachedSet) ||
      !ctx.can_impl_ds(fcfs_cs_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_fcfs_cs_id(fcfs_cs_data->map_objs.map))) {
      return {};
    }
  }

  std::vector<u32> allowed_cache_capacities = enum_fcfs_cs_cache_capacities(fcfs_cs_data->capacity);
  std::sort(allowed_cache_capacities.begin(), allowed_cache_capacities.end(), std::greater<int>());

  // Let's optimistically pick the largest cache capacity that we can build or reuse.
  std::optional<u32> cache_capacity;
  for (u32 cache_capacity_candidate : allowed_cache_capacities) {
    if (can_build_or_reuse_fcfs_cs(ep, node, fcfs_cs_data->obj, fcfs_cs_data->original_key, fcfs_cs_data->capacity, cache_capacity_candidate)) {
      cache_capacity = cache_capacity_candidate;
      break;
    }
  }

  if (!cache_capacity.has_value()) {
    return {};
  }

  Context new_ctx = ctx;

  new_ctx.save_ds_impl(fcfs_cs_data->map_objs.map, DSImpl::Tofino_FCFSCachedSet);
  new_ctx.save_ds_impl(fcfs_cs_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedSet);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_SET_CACHE_SIZE_PARAM, cache_capacity.value()}}), new_ctx);

  return spec_impl;
}

std::vector<impl_t> FCFSCachedSetReadFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const std::optional<fcfs_cs_data_t> fcfs_cs_data = build_fcfs_cs_data(ep->get_bdd(), ep->get_ctx(), map_get);
  if (!fcfs_cs_data.has_value()) {
    return {};
  }

  if (!fcfs_cs_data->map_objs.vectors.empty()) {
    return {};
  }

  if (!ep->get_ctx().is_dchain_used_exclusively_for_linking_maps_with_vectors(fcfs_cs_data->map_objs.dchain)) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(fcfs_cs_data->map_objs.map, DSImpl::Tofino_FCFSCachedSet) ||
      !ep->get_ctx().can_impl_ds(fcfs_cs_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_fcfs_cs_id(fcfs_cs_data->map_objs.map))) {
      return {};
    }
  }

  std::vector<impl_t> impls;
  for (u32 cache_capacity : enum_fcfs_cs_cache_capacities(fcfs_cs_data->capacity)) {
    if (!can_build_or_reuse_fcfs_cs(ep, node, fcfs_cs_data->obj, fcfs_cs_data->original_key, fcfs_cs_data->capacity, cache_capacity)) {
      continue;
    }

    std::unique_ptr<EP> new_ep = concretize(ep, node, fcfs_cs_data.value(), cache_capacity);
    if (new_ep) {
      impl_t impl = implement(ep, node, std::move(new_ep), {{FCFS_CACHED_SET_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(std::move(impl));
    }
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedSetReadFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const std::optional<fcfs_cs_data_t> fcfs_cs_data = build_fcfs_cs_data(bdd, ctx, map_get);
  if (!fcfs_cs_data.has_value()) {
    return {};
  }

  if (!fcfs_cs_data->map_objs.vectors.empty()) {
    return {};
  }

  if (!ctx.is_dchain_used_exclusively_for_linking_maps_with_vectors(fcfs_cs_data->map_objs.dchain)) {
    return {};
  }

  if (!ctx.check_ds_impl(fcfs_cs_data->map_objs.map, DSImpl::Tofino_FCFSCachedSet) ||
      !ctx.check_ds_impl(fcfs_cs_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(fcfs_cs_data->map_objs.map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedSet *fcfs_cs = dynamic_cast<const FCFSCachedSet *>(*ds.begin());

  return std::make_unique<FCFSCachedSetRead>(node, fcfs_cs->id, fcfs_cs_data->obj, fcfs_cs_data->original_key, fcfs_cs_data->keys,
                                             fcfs_cs_data->map_has_this_key);
}

} // namespace Tofino
} // namespace LibSynapse