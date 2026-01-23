#include <LibSynapse/Modules/Tofino/FCFSCachedTableRead.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct fcfs_ct_data_t {
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  symbol_t map_has_this_key;
  u32 capacity;
  map_coalescing_objs_t map_objs;
};

std::optional<fcfs_ct_data_t> build_fcfs_ct_data(const BDD *bdd, const Context &ctx, const Call *map_get) {
  const call_t &call = map_get->get_call();

  fcfs_ct_data_t data;
  data.obj              = expr_addr_to_obj_addr(call.args.at("map").expr);
  data.original_key     = call.args.at("key").in;
  data.keys             = Table::build_keys(data.original_key, ctx.get_expr_structs());
  data.value            = call.args.at("value_out").out;
  data.map_has_this_key = map_get->get_local_symbol("map_has_this_key");
  data.capacity         = ctx.get_map_config(data.obj).capacity;

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(data.obj);
  if (!map_objs.has_value()) {
    return {};
  }
  data.map_objs = map_objs.value();

  return data;
}

std::unique_ptr<EP> concretize(const EP *ep, const BDDNode *node, const fcfs_ct_data_t &fcfs_ct_data, u32 cache_capacity) {
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_ct(new_ep.get(), node, fcfs_ct_data.obj, fcfs_ct_data.original_key,
                                                                              fcfs_ct_data.capacity, cache_capacity);
  if (!cached_table) {
    return nullptr;
  }

  Module *module = new FCFSCachedTableRead(node, cached_table->id, fcfs_ct_data.obj, fcfs_ct_data.original_key, fcfs_ct_data.keys, fcfs_ct_data.value,
                                           fcfs_ct_data.map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), fcfs_ct_data.map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  ctx.save_ds_impl(node->get_id(), fcfs_ct_data.map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, fcfs_ct_data.map_objs.map, cached_table);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return new_ep;
}

} // namespace

std::optional<spec_impl_t> FCFSCachedTableReadFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const std::optional<fcfs_ct_data_t> fcfs_ct_data = build_fcfs_ct_data(ep->get_bdd(), ep->get_ctx(), map_get);
  if (!fcfs_ct_data.has_value()) {
    return {};
  }

  if (!speculations.ctx.can_impl_ds(fcfs_ct_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !speculations.ctx.can_impl_ds(fcfs_ct_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), speculations, node, fcfs_ct_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable,
                          build_fcfs_ct_id(fcfs_ct_data->map_objs.map))) {
    return {};
  }

  std::vector<u32> allowed_cache_capacities = enum_fcfs_ct_cache_capacities(fcfs_ct_data->capacity);
  std::sort(allowed_cache_capacities.begin(), allowed_cache_capacities.end(), std::greater<int>());

  // Let's optimistically pick the largest cache capacity that we can build or reuse.
  std::optional<u32> cache_capacity;
  for (u32 cache_capacity_candidate : allowed_cache_capacities) {
    if (can_build_or_reuse_fcfs_ct(ep, node, fcfs_ct_data->obj, fcfs_ct_data->original_key, fcfs_ct_data->capacity, cache_capacity_candidate)) {
      cache_capacity = cache_capacity_candidate;
      break;
    }
  }

  if (!cache_capacity.has_value()) {
    return {};
  }

  Context new_ctx = speculations.ctx;

  new_ctx.save_ds_impl(node->get_id(), fcfs_ct_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(node->get_id(), fcfs_ct_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity.value()}}), new_ctx);

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

  const std::optional<fcfs_ct_data_t> fcfs_ct_data = build_fcfs_ct_data(ep->get_bdd(), ep->get_ctx(), map_get);
  if (!fcfs_ct_data.has_value()) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(fcfs_ct_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(fcfs_ct_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), build_fcfs_ct_id(fcfs_ct_data->map_objs.map))) {
    return {};
  }

  std::vector<impl_t> impls;
  for (u32 cache_capacity : enum_fcfs_ct_cache_capacities(fcfs_ct_data->capacity)) {
    if (!can_build_or_reuse_fcfs_ct(ep, node, fcfs_ct_data->obj, fcfs_ct_data->original_key, fcfs_ct_data->capacity, cache_capacity)) {
      continue;
    }

    std::unique_ptr<EP> new_ep = concretize(ep, node, fcfs_ct_data.value(), cache_capacity);
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

  const std::optional<fcfs_ct_data_t> fcfs_ct_data = build_fcfs_ct_data(bdd, ctx, map_get);
  if (!fcfs_ct_data.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(fcfs_ct_data->map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.check_ds_impl(fcfs_ct_data->map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(fcfs_ct_data->map_objs.map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedTable *fcfs_ct = dynamic_cast<const FCFSCachedTable *>(*ds.begin());

  return std::make_unique<FCFSCachedTableRead>(node, fcfs_ct->id, fcfs_ct_data->obj, fcfs_ct_data->original_key, fcfs_ct_data->keys,
                                               fcfs_ct_data->value, fcfs_ct_data->map_has_this_key);
}

} // namespace Tofino
} // namespace LibSynapse