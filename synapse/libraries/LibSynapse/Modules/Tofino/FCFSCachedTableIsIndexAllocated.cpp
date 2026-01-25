#include <LibSynapse/Modules/Tofino/FCFSCachedTableIsIndexAllocated.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> FCFSCachedTableIsIndexAllocatedFactory::speculate(const EP *ep, const BDDNode *node,
                                                                             const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return {};
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
  const addr_t obj               = expr_addr_to_obj_addr(obj_expr);

  const std::optional<map_coalescing_objs_t> map_objs = speculations.ctx.get_map_coalescing_objs(obj);
  if (!map_objs.has_value()) {
    return {};
  }

  const u32 capacity = speculations.ctx.get_map_config(map_objs->map).capacity;

  if (!speculations.ctx.can_impl_ds(map_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !speculations.ctx.can_impl_ds(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (speculations.ctx.get_ds_usage_counts().contains({map_objs->map, DSImpl::Tofino_FCFSCachedTable}) &&
      speculations.ctx.get_ds_usage_counts().at({map_objs->map, DSImpl::Tofino_FCFSCachedTable}) >= 2) {
    return {};
  }

  bool requires_recirculation = false;
  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), speculations, node, map_objs->map, DSImpl::Tofino_FCFSCachedTable,
                          build_fcfs_ct_id(map_objs->map))) {
    requires_recirculation = true;
  }

  const std::vector<const Call *> map_gets = ep->get_bdd()->get_map_gets(map_objs->map);
  if (map_gets.empty()) {
    return {};
  }

  klee::ref<klee::Expr> original_key = map_gets.front()->get_call().args.at("key").in;

  std::vector<u32> allowed_cache_capacities = enum_fcfs_ct_cache_capacities(capacity);
  std::sort(allowed_cache_capacities.begin(), allowed_cache_capacities.end(), std::greater<int>());

  // Let's optimistically pick the largest cache capacity that we can build or reuse.
  std::optional<u32> cache_capacity;
  for (u32 cache_capacity_candidate : allowed_cache_capacities) {
    if (can_build_or_reuse_fcfs_ct(ep, node, map_objs->map, original_key, capacity, cache_capacity_candidate, false)) {
      cache_capacity = cache_capacity_candidate;
      break;
    }
  }

  if (!cache_capacity.has_value()) {
    return {};
  }

  Context new_ctx = speculations.ctx;

  if (requires_recirculation) {
    new_ctx.get_mutable_perf_oracle().add_recirculated_traffic(
        ep->get_speculative_node_egress(new_ctx.get_profiler().get_hr(node), node, speculations));
  }

  new_ctx.save_ds_impl(node->get_id(), map_objs->map, DSImpl::Tofino_FCFSCachedTable);
  new_ctx.save_ds_impl(node->get_id(), map_objs->dchain, DSImpl::Tofino_FCFSCachedTable);

  spec_impl_t spec_impl(decide(ep, node, {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity.value()}}), new_ctx);
  spec_impl.recirculated = requires_recirculation;

  return spec_impl;
}

std::vector<impl_t> FCFSCachedTableIsIndexAllocatedFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return {};
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index    = call.args.at("index").expr;

  const addr_t obj = expr_addr_to_obj_addr(obj_expr);

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(map_objs->map, DSImpl::Tofino_FCFSCachedTable) ||
      !ep->get_ctx().can_impl_ds(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  if (ep->get_ctx().get_ds_usage_counts().contains({map_objs->map, DSImpl::Tofino_FCFSCachedTable}) &&
      ep->get_ctx().get_ds_usage_counts().at({map_objs->map, DSImpl::Tofino_FCFSCachedTable}) >= 2) {
    return {};
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), build_fcfs_ct_id(map_objs->map))) {
    return {};
  }

  const u32 capacity          = ep->get_ctx().get_map_config(map_objs->map).capacity;
  const symbol_t is_allocated = call_node->get_local_symbol("is_index_allocated");

  const std::vector<const Call *> map_gets = ep->get_bdd()->get_map_gets(map_objs->map);
  if (map_gets.empty()) {
    return {};
  }

  klee::ref<klee::Expr> original_key = map_gets.front()->get_call().args.at("key").in;

  std::vector<impl_t> impls;

  for (u32 cache_capacity : enum_fcfs_ct_cache_capacities(capacity)) {
    if (!can_build_or_reuse_fcfs_ct(ep, node, map_objs->map, original_key, capacity, cache_capacity, false)) {
      continue;
    }

    std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

    FCFSCachedTable *fcfs_ct = build_or_reuse_fcfs_ct(new_ep.get(), node, map_objs->map, original_key, capacity, cache_capacity, false);
    if (!fcfs_ct) {
      continue;
    }

    Module *module  = new FCFSCachedTableIsIndexAllocated(node, fcfs_ct->id, map_objs->map, index, is_allocated);
    EPNode *ep_node = new EPNode(module);

    const EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(node->get_id(), map_objs->map, DSImpl::Tofino_FCFSCachedTable);
    new_ep->get_mutable_ctx().save_ds_impl(node->get_id(), map_objs->dchain, DSImpl::Tofino_FCFSCachedTable);

    TofinoContext *tofino_ctx = TofinoModuleFactory::get_mutable_tofino_ctx(new_ep.get());
    tofino_ctx->place(new_ep.get(), node, map_objs->map, fcfs_ct);

    impl_t impl = implement(ep, node, std::move(new_ep), {{FCFS_CACHED_TABLE_CACHE_SIZE_PARAM, cache_capacity}});
    impls.push_back(std::move(impl));
  }

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableIsIndexAllocatedFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return {};
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index    = call.args.at("index").expr;

  const addr_t obj            = expr_addr_to_obj_addr(obj_expr);
  const symbol_t is_allocated = call_node->get_local_symbol("is_index_allocated");

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.can_impl_ds(map_objs->map, DSImpl::Tofino_FCFSCachedTable) || !ctx.can_impl_ds(map_objs->dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(map_objs->map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const FCFSCachedTable *fcfs_ct = dynamic_cast<const FCFSCachedTable *>(*ds.begin());

  return std::make_unique<FCFSCachedTableIsIndexAllocated>(node, fcfs_ct->id, map_objs->map, index, is_allocated);
}

} // namespace Tofino
} // namespace LibSynapse