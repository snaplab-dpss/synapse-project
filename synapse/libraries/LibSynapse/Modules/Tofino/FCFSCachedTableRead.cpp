#include <LibSynapse/Modules/Tofino/FCFSCachedTableRead.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {

struct fcfs_cached_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  LibCore::symbol_t map_has_this_key;
  u32 capacity;

  fcfs_cached_table_data_t(const Context &ctx, const LibBDD::Call *map_get) {
    const LibBDD::call_t &call = map_get->get_call();

    obj              = LibCore::expr_addr_to_obj_addr(call.args.at("map").expr);
    key              = call.args.at("key").in;
    read_value       = call.args.at("value_out").out;
    map_has_this_key = map_get->get_local_symbol("map_has_this_key");
    capacity         = ctx.get_map_config(obj).capacity;
  }
};

EP *concretize_cached_table_read(const EP *ep, const LibBDD::Node *node, const LibBDD::map_coalescing_objs_t &map_objs,
                                 const fcfs_cached_table_data_t &cached_table_data, u32 cache_capacity) {
  FCFSCachedTable *cached_table = TofinoModuleFactory::build_or_reuse_fcfs_cached_table(
      ep, node, cached_table_data.obj, cached_table_data.key, cached_table_data.capacity, cache_capacity);

  if (!cached_table) {
    return nullptr;
  }

  Module *module  = new FCFSCachedTableRead(node, cached_table->id, cached_table->tables.back().id, cached_table_data.obj,
                                            cached_table_data.key, cached_table_data.read_value, cached_table_data.map_has_this_key);
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

std::optional<spec_impl_t> FCFSCachedTableReadFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return std::nullopt;
  }

  const fcfs_cached_table_data_t cached_table_data(ep->get_ctx(), map_get);

  const std::optional<LibBDD::map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(cached_table_data.obj);
  if (!map_objs.has_value()) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.can_impl_ds(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return std::nullopt;
  }

  FCFSCachedTable *fcfs_cached_table = get_fcfs_cached_table(ep, node, cached_table_data.obj);
  if (!fcfs_cached_table) {
    return std::nullopt;
  }

  Context new_ctx = ctx;

  new_ctx.save_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, fcfs_cached_table->cache_capacity}}), new_ctx);

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableReadFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                             LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return impls;
  }

  const fcfs_cached_table_data_t cached_table_data(ep->get_ctx(), map_get);

  const std::optional<LibBDD::map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(cached_table_data.obj);
  if (!map_objs.has_value()) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return impls;
  }

  const std::vector<u32> allowed_cache_capacities = enum_fcfs_cache_cap(cached_table_data.capacity);

  for (u32 cache_capacity : allowed_cache_capacities) {
    EP *new_ep = concretize_cached_table_read(ep, node, map_objs.value(), cached_table_data, cache_capacity);

    if (new_ep) {
      impl_t impl = implement(ep, node, new_ep, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
      impls.push_back(impl);
    }
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableReadFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const fcfs_cached_table_data_t cached_table_data(ctx, map_get);

  const std::optional<LibBDD::map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(cached_table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_ds(map_objs->map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedTable *fcfs_cached_table = dynamic_cast<const FCFSCachedTable *>(*ds.begin());

  return std::make_unique<FCFSCachedTableRead>(node, fcfs_cached_table->id, fcfs_cached_table->tables.back().id, cached_table_data.obj,
                                               cached_table_data.key, cached_table_data.read_value, cached_table_data.map_has_this_key);
}

} // namespace Tofino
} // namespace LibSynapse