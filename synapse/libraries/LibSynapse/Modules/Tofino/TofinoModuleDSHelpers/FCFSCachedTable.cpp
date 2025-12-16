#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

FCFSCachedTable *build_fcfs_cached_table(const EP *ep, const BDDNode *node, DS_ID id, klee::ref<klee::Expr> key, u32 capacity, u32 cache_capacity) {
  const Context &ctx                 = ep->get_ctx();
  const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna                     = tofino_ctx->get_tna();
  const tna_properties_t &properties = tna.tna_config.properties;

  const std::vector<klee::ref<klee::Expr>> keys = Register::partition_value(properties, key, ctx.get_expr_structs());

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> k : keys) {
    keys_sizes.push_back(k->getWidth());
  }

  FCFSCachedTable *fcfs_cached_table = new FCFSCachedTable(properties, id, node->get_id(), cache_capacity, capacity, keys_sizes);

  if (!tofino_ctx->can_place(ep, node, fcfs_cached_table)) {
    delete fcfs_cached_table;
    fcfs_cached_table = nullptr;
  }

  return fcfs_cached_table;
}

FCFSCachedTable *internal_get_fcfs_cached_table(const EP *ep, const BDDNode *node, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->get_data_structures().has(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::FCFSCachedTable && "Invalid DS type");

  return dynamic_cast<FCFSCachedTable *>(*ds.begin());
}

FCFSCachedTable *reuse_fcfs_cached_table(const EP *ep, const BDDNode *node, addr_t obj, u32 cache_capacity) {
  FCFSCachedTable *fcfs_cached_table = internal_get_fcfs_cached_table(ep, node, obj);
  assert(fcfs_cached_table && "FCFS cached table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!fcfs_cached_table->has_table(node->get_id()));

  if (fcfs_cached_table->cache_capacity != cache_capacity) {
    return nullptr;
  }

  std::optional<DS_ID> added_table_id;
  if (!fcfs_cached_table->has_table(node->get_id())) {
    added_table_id = fcfs_cached_table->add_table(node->get_id());
  }

  if (!tofino_ctx->can_place(ep, node, fcfs_cached_table)) {
    if (added_table_id.has_value()) {
      fcfs_cached_table->remove_table(added_table_id.value());
    }
    fcfs_cached_table = nullptr;
  }

  return fcfs_cached_table;
}

} // namespace

DS_ID TofinoModuleFactory::build_fcfs_cached_table_id(addr_t obj) { return "fcfs_cached_table_" + std::to_string(obj); }

FCFSCachedTable *TofinoModuleFactory::build_or_reuse_fcfs_cached_table(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key,
                                                                       u32 capacity, u32 cache_capacity) {
  FCFSCachedTable *fcfs_cached_table = nullptr;

  const Context &ctx        = ep->get_ctx();
  const bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    fcfs_cached_table = reuse_fcfs_cached_table(ep, node, obj, cache_capacity);
  } else {
    const DS_ID id    = build_fcfs_cached_table_id(obj);
    fcfs_cached_table = build_fcfs_cached_table(ep, node, id, key, capacity, cache_capacity);
  }

  return fcfs_cached_table;
}

bool TofinoModuleFactory::can_reuse_fcfs_cached_table(const EP *ep, const BDDNode *node, addr_t obj, u32 cache_capacity) {
  FCFSCachedTable *fcfs_cached_table = internal_get_fcfs_cached_table(ep, node, obj);
  assert(fcfs_cached_table && "FCFS cached table not found");

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  assert(!fcfs_cached_table->has_table(node->get_id()));

  if (fcfs_cached_table->cache_capacity != cache_capacity) {
    return false;
  }

  std::unique_ptr<FCFSCachedTable> clone = std::unique_ptr<FCFSCachedTable>(dynamic_cast<FCFSCachedTable *>(fcfs_cached_table->clone()));
  clone->add_table(node->get_id());

  return tofino_ctx->can_place(ep, node, clone.get());
}

bool TofinoModuleFactory::can_build_or_reuse_fcfs_cached_table(const EP *ep, const BDDNode *node, addr_t obj, klee::ref<klee::Expr> key, u32 capacity,
                                                               u32 cache_capacity) {
  const Context &ctx        = ep->get_ctx();
  const bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    return can_reuse_fcfs_cached_table(ep, node, obj, cache_capacity);
  }

  const DS_ID id                     = build_fcfs_cached_table_id(obj);
  FCFSCachedTable *fcfs_cached_table = build_fcfs_cached_table(ep, node, id, key, capacity, cache_capacity);

  if (!fcfs_cached_table) {
    return false;
  }

  delete fcfs_cached_table;
  return true;
}

FCFSCachedTable *TofinoModuleFactory::get_fcfs_cached_table(const EP *ep, const BDDNode *node, addr_t obj) {
  return internal_get_fcfs_cached_table(ep, node, obj);
}

std::vector<u32> TofinoModuleFactory::enum_fcfs_cache_cap(u32 capacity) {
  std::vector<u32> capacities;

  u32 cache_capacity = 8;
  while (cache_capacity < capacity) {
    capacities.push_back(cache_capacity);
    cache_capacity *= 2;

    // Overflow check
    assert((capacities.empty() || capacities.back() < cache_capacity) && "Overflow");
  }

  return capacities;
}

hit_rate_t TofinoModuleFactory::get_fcfs_cache_success_rate(const Context &ctx, const BDDNode *node, klee::ref<klee::Expr> key, u32 cache_capacity) {
  const flow_stats_t flow_stats = ctx.get_profiler().get_flow_stats(node, key);
  const u64 avg_pkts_per_flow   = flow_stats.pkts / flow_stats.flows;
  const u64 cached_packets      = std::min(flow_stats.pkts, avg_pkts_per_flow * cache_capacity);
  const hit_rate_t hit_rate(cached_packets, flow_stats.pkts);

  // std::cerr << "avg_pkts_per_flow: " << avg_pkts_per_flow << std::endl;
  // std::cerr << "cached_packets: " << cached_packets << std::endl;
  // std::cerr << "hit_rate: " << hit_rate << std::endl;
  // dbg_pause();

  return hit_rate;
}

} // namespace Tofino
} // namespace LibSynapse