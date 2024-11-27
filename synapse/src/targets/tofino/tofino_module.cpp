#include "tofino_module.h"

namespace tofino {

static std::unordered_set<DS *> build_vector_registers(
    const EP *ep, const Node *node, const vector_register_data_t &data,
    std::unordered_set<DS_ID> &rids, std::unordered_set<DS_ID> &deps) {
  std::unordered_set<DS *> regs;

  const TofinoContext *tofino_ctx =
      ep->get_ctx().get_target_ctx<TofinoContext>();
  const TNAProperties &properties = tofino_ctx->get_tna().get_properties();

  std::vector<klee::ref<klee::Expr>> partitions =
      Register::partition_value(properties, data.value);

  for (klee::ref<klee::Expr> partition : partitions) {
    DS_ID rid = "vector_" + std::to_string(node->get_id()) + "_" +
                std::to_string(rids.size());
    bits_t partition_size = partition->getWidth();
    Register *reg =
        new Register(properties, rid, data.num_entries, data.index->getWidth(),
                     partition_size, data.actions);
    regs.insert(reg);
    rids.insert(rid);
  }

  if (!tofino_ctx->check_many_placements(ep, node, {regs})) {
    for (DS *reg : regs) {
      delete reg;
    }
    regs.clear();
  }

  return regs;
}

static std::unordered_set<DS *> get_vector_registers(
    const EP *ep, const Node *node, const vector_register_data_t &data,
    std::unordered_set<DS_ID> &rids, std::unordered_set<DS_ID> &deps) {
  std::unordered_set<DS *> regs;

  const TofinoContext *tofino_ctx =
      ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(data.obj)) {
    return regs;
  }

  regs = tofino_ctx->get_ds(data.obj);
  assert(regs.size());

  for (DS *reg : regs) {
    assert(reg->type == DSType::REGISTER);
    rids.insert(reg->id);
  }

  if (!tofino_ctx->check_many_placements(ep, node, {regs})) {
    regs.clear();
  }

  return regs;
}

std::unordered_set<DS *> TofinoModuleGenerator::build_or_reuse_vector_registers(
    const EP *ep, const Node *node, const vector_register_data_t &data,
    bool &already_exists, std::unordered_set<DS_ID> &rids,
    std::unordered_set<DS_ID> &deps) const {
  std::unordered_set<DS *> regs;

  const Context &ctx = ep->get_ctx();
  bool regs_already_placed =
      ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (regs_already_placed) {
    regs = get_vector_registers(ep, node, data, rids, deps);
    already_exists = true;
  } else {
    regs = build_vector_registers(ep, node, data, rids, deps);
    already_exists = false;
  }

  return regs;
}

bool TofinoModuleGenerator::can_build_or_reuse_vector_registers(
    const EP *ep, const Node *node, const vector_register_data_t &data) const {
  std::unordered_set<DS_ID> rids;
  std::unordered_set<DS_ID> deps;

  const Context &ctx = ep->get_ctx();
  bool regs_already_placed =
      ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (regs_already_placed) {
    std::unordered_set<DS *> regs =
        get_vector_registers(ep, node, data, rids, deps);
    return !regs.empty();
  }

  std::unordered_set<DS *> regs =
      build_vector_registers(ep, node, data, rids, deps);
  bool success = !regs.empty();

  for (DS *reg : regs) {
    delete reg;
  }

  return success;
}

static FCFSCachedTable *build_fcfs_cached_table(const EP *ep, const Node *node,
                                                klee::ref<klee::Expr> key,
                                                u32 num_entries,
                                                u32 cache_capacity) {
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna = tofino_ctx->get_tna();
  const TNAProperties &properties = tna.get_properties();

  DS_ID id = "fcfs_cached_table_" + std::to_string(cache_capacity);

  std::vector<klee::ref<klee::Expr>> keys =
      Register::partition_value(properties, key);
  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  FCFSCachedTable *cached_table = new FCFSCachedTable(
      properties, id, node->get_id(), cache_capacity, num_entries, keys_sizes);

  if (!tofino_ctx->check_placement(ep, node, cached_table)) {
    delete cached_table;
    cached_table = nullptr;
  }

  return cached_table;
}

static FCFSCachedTable *reuse_fcfs_cached_table(const EP *ep, const Node *node,
                                                addr_t obj) {
  const TofinoContext *tofino_ctx =
      ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::FCFS_CACHED_TABLE);

  FCFSCachedTable *cached_table = static_cast<FCFSCachedTable *>(*ds.begin());

  if (!cached_table->has_table(node->get_id())) {
    FCFSCachedTable *clone =
        static_cast<FCFSCachedTable *>(cached_table->clone());
    clone->add_table(node->get_id());
    cached_table = clone;
  }

  if (!tofino_ctx->check_placement(ep, node, cached_table)) {
    cached_table = nullptr;
  }

  return cached_table;
}

FCFSCachedTable *TofinoModuleGenerator::build_or_reuse_fcfs_cached_table(
    const EP *ep, const Node *node, addr_t obj, klee::ref<klee::Expr> key,
    u32 num_entries, u32 cache_capacity) const {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = reuse_fcfs_cached_table(ep, node, obj);
  } else {
    cached_table =
        build_fcfs_cached_table(ep, node, key, num_entries, cache_capacity);
  }

  return cached_table;
}

FCFSCachedTable *
TofinoModuleGenerator::get_fcfs_cached_table(const EP *ep, const Node *node,
                                             addr_t obj) const {
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::FCFS_CACHED_TABLE);

  FCFSCachedTable *cached_table = static_cast<FCFSCachedTable *>(*ds.begin());

  if (!tofino_ctx->check_placement(ep, node, cached_table)) {
    cached_table = nullptr;
  }

  return cached_table;
}

bool TofinoModuleGenerator::can_get_or_build_fcfs_cached_table(
    const EP *ep, const Node *node, addr_t obj, klee::ref<klee::Expr> key,
    u32 num_entries, u32 cache_capacity) const {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = get_fcfs_cached_table(ep, node, obj);

    if (!cached_table || cached_table->cache_capacity != cache_capacity) {
      return false;
    }

    return true;
  }

  cached_table =
      build_fcfs_cached_table(ep, node, key, num_entries, cache_capacity);

  if (!cached_table) {
    return false;
  }

  delete cached_table;
  return true;
}

symbols_t TofinoModuleGenerator::get_dataplane_state(const EP *ep,
                                                     const Node *node) const {
  const nodes_t &roots = ep->get_target_roots(TargetType::Tofino);
  return get_prev_symbols(node, roots);
}

std::vector<u32>
TofinoModuleGenerator::enum_fcfs_cache_cap(u32 num_entries) const {
  std::vector<u32> capacities;

  u32 cache_capacity = 8;
  while (1) {
    capacities.push_back(cache_capacity);
    cache_capacity *= 2;

    // Overflow check
    assert(capacities.empty() || capacities.back() < cache_capacity);

    if (cache_capacity > num_entries) {
      break;
    }
  }

  return capacities;
}

hit_rate_t TofinoModuleGenerator::get_fcfs_cache_success_rate(
    const Context &ctx, const Node *node, klee::ref<klee::Expr> key,
    u32 cache_capacity) const {
  constraints_t constraints = node->get_ordered_branch_constraints();
  FlowStats flow_stats = ctx.get_profiler().get_flow_stats(constraints, key);

  u64 avg_pkts_per_flow = flow_stats.pkts / flow_stats.flows;
  u64 cached_packets =
      std::min(flow_stats.pkts, avg_pkts_per_flow * cache_capacity);
  hit_rate_t hit_rate =
      flow_stats.pkts == 0
          ? 0
          : cached_packets / static_cast<hit_rate_t>(flow_stats.pkts);

  // std::cerr << "node: " << node->dump(true, true) << "\n";
  // std::cerr << "avg_pkts_per_flow: " << avg_pkts_per_flow << std::endl;
  // std::cerr << "cached_packets: " << cached_packets << std::endl;
  // std::cerr << "hit_rate: " << hit_rate << std::endl;
  // DEBUG_PAUSE

  return hit_rate;
}

static HHTable *build_hh_table(const EP *ep, const Node *node,
                               const std::vector<klee::ref<klee::Expr>> &keys,
                               u32 num_entries, u32 cms_width, u32 cms_height) {

  DS_ID id = "hh_table_" + std::to_string(cms_width) + "x" +
             std::to_string(cms_height);

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  HHTable *hh_table = new HHTable(id, node->get_id(), num_entries, cms_width,
                                  cms_height, keys_sizes);

  const TofinoContext *tofino_ctx =
      ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->check_placement(ep, node, hh_table)) {
    delete hh_table;
    hh_table = nullptr;
  }

  return hh_table;
}

static HHTable *reuse_hh_table(const EP *ep, const Node *node, addr_t obj) {
  const Context &ctx = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::HH_TABLE);

  HHTable *hh_table = static_cast<HHTable *>(*ds.begin());

  if (!tofino_ctx->check_placement(ep, node, hh_table)) {
    hh_table = nullptr;
  }

  return hh_table;
}

HHTable *TofinoModuleGenerator::build_or_reuse_hh_table(
    const EP *ep, const Node *node, addr_t obj,
    const std::vector<klee::ref<klee::Expr>> &keys, u32 num_entries,
    u32 cms_width, u32 cms_height) const {
  HHTable *hh_table = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    hh_table = reuse_hh_table(ep, node, obj);
  } else {
    hh_table =
        build_hh_table(ep, node, keys, num_entries, cms_width, cms_height);
  }

  return hh_table;
}

bool TofinoModuleGenerator::can_build_or_reuse_hh_table(
    const EP *ep, const Node *node, addr_t obj,
    const std::vector<klee::ref<klee::Expr>> &keys, u32 num_entries,
    u32 cms_width, u32 cms_height) const {
  HHTable *hh_table = nullptr;

  const Context &ctx = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable);

  if (already_placed) {
    const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
    const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

    assert(ds.size() == 1);
    assert((*ds.begin())->type == DSType::HH_TABLE);

    hh_table = static_cast<HHTable *>(*ds.begin());

    if (!tofino_ctx->check_placement(ep, node, hh_table)) {
      hh_table = nullptr;
      return false;
    }

    if (hh_table->cms_width != cms_width ||
        hh_table->cms_height != cms_height) {
      return false;
    }

    return true;
  }

  hh_table = build_hh_table(ep, node, keys, num_entries, cms_width, cms_height);

  if (!hh_table) {
    return false;
  }

  delete hh_table;
  return true;
}

hit_rate_t TofinoModuleGenerator::get_hh_table_hit_success_rate(
    const Context &ctx, const Node *node, klee::ref<klee::Expr> key,
    u32 capacity) const {
  constraints_t constraints = node->get_ordered_branch_constraints();
  FlowStats flow_stats = ctx.get_profiler().get_flow_stats(constraints, key);

  u64 top_k = 0;
  for (size_t k = 0; k <= capacity && k < flow_stats.pkts_per_flow.size();
       k++) {
    top_k += flow_stats.pkts_per_flow[k];
  }

  assert(top_k <= flow_stats.pkts);
  hit_rate_t hit_rate = top_k / static_cast<hit_rate_t>(flow_stats.pkts);

  return hit_rate;
}

static CountMinSketch *build_cms(const EP *ep, const Node *node, u32 width,
                                 u32 height) {
  const TofinoContext *tofino_ctx =
      ep->get_ctx().get_target_ctx<TofinoContext>();

  DS_ID id = "cms_" + std::to_string(width) + "x" + std::to_string(height);
  const TNAProperties &properties = tofino_ctx->get_tna().get_properties();

  CountMinSketch *cms = new CountMinSketch(properties, id, width, height);

  if (!tofino_ctx->check_placement(ep, node, cms)) {
    delete cms;
    cms = nullptr;
  }

  return cms;
}

static CountMinSketch *reuse_cms(const EP *ep, const Node *node, addr_t obj) {
  const TofinoContext *tofino_ctx =
      ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::COUNT_MIN_SKETCH);

  CountMinSketch *cms = static_cast<CountMinSketch *>(*ds.begin());

  if (!tofino_ctx->check_placement(ep, node, cms)) {
    return nullptr;
  }

  return cms;
}

bool TofinoModuleGenerator::can_build_or_reuse_cms(const EP *ep,
                                                   const Node *node, addr_t obj,
                                                   u32 width,
                                                   u32 height) const {
  CountMinSketch *cms = nullptr;

  const Context &ctx = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_CountMinSketch);

  if (already_placed) {
    const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
    const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

    assert(ds.size() == 1);
    assert((*ds.begin())->type == DSType::COUNT_MIN_SKETCH);

    cms = static_cast<CountMinSketch *>(*ds.begin());

    if (!tofino_ctx->check_placement(ep, node, cms)) {
      cms = nullptr;
      return false;
    }

    if (cms->width != width || cms->height != height) {
      return false;
    }

    return true;
  }

  cms = build_cms(ep, node, width, height);

  if (!cms) {
    return false;
  }

  delete cms;
  return true;
}

CountMinSketch *TofinoModuleGenerator::build_or_reuse_cms(const EP *ep,
                                                          const Node *node,
                                                          addr_t obj, u32 width,
                                                          u32 height) const {
  CountMinSketch *cms = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_CountMinSketch)) {
    cms = reuse_cms(ep, node, obj);
  } else {
    cms = build_cms(ep, node, width, height);
  }

  return cms;
}

} // namespace tofino
