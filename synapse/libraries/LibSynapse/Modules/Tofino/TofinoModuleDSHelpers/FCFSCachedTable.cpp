#include <LibSynapse/Modules/Tofino/TofinoModule.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>

#include <unordered_set>

namespace LibSynapse {
namespace Tofino {

namespace {

FCFSCachedTable *build_fcfs_cached_table(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj, klee::ref<klee::Expr> key,
                                         u32 capacity, u32 cache_capacity) {
  const Context &ctx                 = ep->get_ctx();
  const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>(type);
  const TNA &tna                     = tofino_ctx->get_tna();
  const tna_properties_t &properties = tna.tna_config.properties;

  const DS_ID id                                = "fcfs_cached_table_" + std::to_string(cache_capacity) + "_" + std::to_string(obj);
  const std::vector<klee::ref<klee::Expr>> keys = Register::partition_value(properties, key, ctx.get_expr_structs());

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> k : keys) {
    keys_sizes.push_back(k->getWidth());
  }

  FCFSCachedTable *cached_table = new FCFSCachedTable(properties, id, node->get_id(), cache_capacity, capacity, keys_sizes);

  if (!tofino_ctx->can_place(ep, node, cached_table)) {
    delete cached_table;
    cached_table = nullptr;
  }

  return cached_table;
}

FCFSCachedTable *reuse_fcfs_cached_table(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>(type);

  if (!tofino_ctx->get_data_structures().has(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::FCFSCachedTable && "Invalid DS type");

  FCFSCachedTable *cached_table = dynamic_cast<FCFSCachedTable *>(*ds.begin());

  if (!cached_table->has_table(node->get_id())) {
    FCFSCachedTable *clone = dynamic_cast<FCFSCachedTable *>(cached_table->clone());
    clone->add_table(node->get_id());
    cached_table = clone;
  }

  if (!tofino_ctx->can_place(ep, node, cached_table)) {
    cached_table = nullptr;
  }

  return cached_table;
}

} // namespace

FCFSCachedTable *TofinoModuleFactory::build_or_reuse_fcfs_cached_table(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj,
                                                                       klee::ref<klee::Expr> key, u32 capacity, u32 cache_capacity) {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = reuse_fcfs_cached_table(ep, node, type, obj);
  } else {
    cached_table = build_fcfs_cached_table(ep, node, type, obj, key, capacity, cache_capacity);
  }

  return cached_table;
}

FCFSCachedTable *TofinoModuleFactory::get_fcfs_cached_table(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>(type);

  if (!tofino_ctx->get_data_structures().has(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_data_structures().get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::FCFSCachedTable && "Invalid DS type");

  return dynamic_cast<FCFSCachedTable *>(*ds.begin());
}

bool TofinoModuleFactory::can_get_or_build_fcfs_cached_table(const EP *ep, const BDDNode *node, const TargetType type, addr_t obj,
                                                             klee::ref<klee::Expr> key, u32 capacity, u32 cache_capacity) {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = get_fcfs_cached_table(ep, node, type, obj);
    assert(false && "FIXME: we should check if we can reuse this data structure, as it requires another table will have to be created");

    if (!cached_table || cached_table->cache_capacity != cache_capacity) {
      return false;
    }

    return true;
  }

  cached_table = build_fcfs_cached_table(ep, node, type, obj, key, capacity, cache_capacity);

  if (!cached_table) {
    return false;
  }

  delete cached_table;
  return true;
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

} // namespace Tofino
} // namespace LibSynapse
