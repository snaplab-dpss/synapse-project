#include "tofino_module.h"
#include "../../execution_plan/execution_plan.h"

#include <unordered_set>
#include <klee/util/ExprVisitor.h>

namespace synapse {
namespace tofino {
namespace {
class ActionExprCompatibilityChecker : public klee::ExprVisitor::ExprVisitor {
private:
  std::unordered_set<std::string> symbols;
  int operations;
  bool compatible;

public:
  ActionExprCompatibilityChecker() : operations(0), compatible(true) {}

  bool is_compatible() const { return compatible; }

  Action visit_incompatible_op() {
    compatible = false;
    std::cerr << "Is incompatible op!\n";
    return Action::skipChildren();
  }

  Action visit_compatible_op() {
    operations++;
    if (operations > 1) {
      compatible = false;
      std::cerr << "Too many ops!\n";
      return Action::skipChildren();
    }
    return Action::doChildren();
  }

  Action visitRead(const klee::ReadExpr &e) override final {
    const std::string name = e.updates.root->name;
    symbols.insert(name);
    if (symbols.size() > 1) {
      compatible = false;
      std::cerr << "Too many symbols!\n";
      return Action::skipChildren();
    }
    return Action::doChildren();
  }

  Action visitConcat(const klee::ConcatExpr &e) override final { return Action::doChildren(); }

  Action visitSelect(const klee::SelectExpr &e) override final { return visit_compatible_op(); }
  Action visitExtract(const klee::ExtractExpr &e) override final { return visit_compatible_op(); }
  Action visitZExt(const klee::ZExtExpr &e) override final { return visit_compatible_op(); }
  Action visitSExt(const klee::SExtExpr &e) override final { return visit_compatible_op(); }
  Action visitAdd(const klee::AddExpr &e) override final { return visit_compatible_op(); }
  Action visitSub(const klee::SubExpr &e) override final { return visit_compatible_op(); }
  Action visitNot(const klee::NotExpr &e) override final { return visit_compatible_op(); }
  Action visitAnd(const klee::AndExpr &e) override final { return visit_compatible_op(); }
  Action visitOr(const klee::OrExpr &e) override final { return visit_compatible_op(); }
  Action visitXor(const klee::XorExpr &e) override final { return visit_compatible_op(); }
  Action visitShl(const klee::ShlExpr &e) override final { return visit_compatible_op(); }
  Action visitLShr(const klee::LShrExpr &e) override final { return visit_compatible_op(); }
  Action visitAShr(const klee::AShrExpr &e) override final { return visit_compatible_op(); }
  Action visitEq(const klee::EqExpr &e) override final { return visit_compatible_op(); }
  Action visitNe(const klee::NeExpr &e) override final { return visit_compatible_op(); }
  Action visitUlt(const klee::UltExpr &e) override final { return visit_compatible_op(); }
  Action visitUle(const klee::UleExpr &e) override final { return visit_compatible_op(); }
  Action visitUgt(const klee::UgtExpr &e) override final { return visit_compatible_op(); }
  Action visitUge(const klee::UgeExpr &e) override final { return visit_compatible_op(); }
  Action visitSlt(const klee::SltExpr &e) override final { return visit_compatible_op(); }
  Action visitSle(const klee::SleExpr &e) override final { return visit_compatible_op(); }
  Action visitSgt(const klee::SgtExpr &e) override final { return visit_compatible_op(); }
  Action visitSge(const klee::SgeExpr &e) override final { return visit_compatible_op(); }

  Action visitMul(const klee::MulExpr &e) override final { return visit_incompatible_op(); }
  Action visitUDiv(const klee::UDivExpr &e) override final { return visit_incompatible_op(); }
  Action visitSDiv(const klee::SDivExpr &e) override final { return visit_incompatible_op(); }
  Action visitURem(const klee::URemExpr &e) override final { return visit_incompatible_op(); }
  Action visitSRem(const klee::SRemExpr &e) override final { return visit_incompatible_op(); }
};

std::unordered_set<Register *> build_vector_registers(const EP *ep, const Node *node,
                                                      const vector_register_data_t &data) {
  std::unordered_set<Register *> regs;

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  const TNAProperties &properties = tofino_ctx->get_tna().get_properties();

  std::vector<klee::ref<klee::Expr>> partitions = Register::partition_value(properties, data.value);

  for (klee::ref<klee::Expr> partition : partitions) {
    DS_ID id              = "vector_reg_" + std::to_string(data.obj) + "_" + std::to_string(regs.size());
    bits_t partition_size = partition->getWidth();
    Register *reg =
        new Register(properties, id, data.num_entries, data.index->getWidth(), partition_size, data.actions);
    regs.insert(reg);
  }

  if (!tofino_ctx->check_many_placements(ep, node, {std::unordered_set<DS *>(regs.begin(), regs.end())})) {
    for (Register *reg : regs) {
      delete reg;
    }
    regs.clear();
  }

  return regs;
}

std::unordered_set<Register *> get_vector_registers(const EP *ep, const Node *node,
                                                    const vector_register_data_t &data) {
  std::unordered_set<Register *> regs;

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(data.obj)) {
    return regs;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(data.obj);
  assert(!ds.empty() && "No vector registers found");

  if (!tofino_ctx->check_many_placements(ep, node, {ds})) {
    return regs;
  }

  for (DS *reg : ds) {
    assert(reg->type == DSType::REGISTER && "Unexpected type");
    regs.insert(dynamic_cast<Register *>(reg));
  }

  return regs;
}

FCFSCachedTable *build_fcfs_cached_table(const EP *ep, const Node *node, addr_t obj, klee::ref<klee::Expr> key,
                                         u32 num_entries, u32 cache_capacity) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  const TNA &tna                  = tofino_ctx->get_tna();
  const TNAProperties &properties = tna.get_properties();

  DS_ID id = "fcfs_cached_table_" + std::to_string(cache_capacity) + "_" + std::to_string(obj);

  std::vector<klee::ref<klee::Expr>> keys = Register::partition_value(properties, key);
  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  FCFSCachedTable *cached_table =
      new FCFSCachedTable(properties, id, node->get_id(), cache_capacity, num_entries, keys_sizes);

  if (!tofino_ctx->check_placement(ep, node, cached_table)) {
    delete cached_table;
    cached_table = nullptr;
  }

  return cached_table;
}

FCFSCachedTable *reuse_fcfs_cached_table(const EP *ep, const Node *node, addr_t obj) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::FCFS_CACHED_TABLE && "Invalid DS type");

  FCFSCachedTable *cached_table = dynamic_cast<FCFSCachedTable *>(*ds.begin());

  if (!cached_table->has_table(node->get_id())) {
    FCFSCachedTable *clone = dynamic_cast<FCFSCachedTable *>(cached_table->clone());
    clone->add_table(node->get_id());
    cached_table = clone;
  }

  if (!tofino_ctx->check_placement(ep, node, cached_table)) {
    cached_table = nullptr;
  }

  return cached_table;
}

HHTable *build_hh_table(const EP *ep, const Node *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys,
                        u32 num_entries, u32 cms_width, u32 cms_height) {

  DS_ID id = "hh_table_" + std::to_string(cms_width) + "x" + std::to_string(cms_height) + "_" + std::to_string(obj);

  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();
  const TNAProperties &properties = tofino_ctx->get_tna().get_properties();

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  HHTable *hh_table = new HHTable(properties, id, node->get_id(), num_entries, keys_sizes, cms_width, cms_height);

  if (!tofino_ctx->check_placement(ep, node, hh_table)) {
    delete hh_table;
    hh_table = nullptr;
  }

  return hh_table;
}

HHTable *reuse_hh_table(const EP *ep, const Node *node, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::HH_TABLE && "Invalid DS type");

  HHTable *hh_table = dynamic_cast<HHTable *>(*ds.begin());

  if (!tofino_ctx->check_placement(ep, node, hh_table)) {
    hh_table = nullptr;
  }

  return hh_table;
}

CountMinSketch *build_cms(const EP *ep, const Node *node, addr_t obj, const std::vector<klee::ref<klee::Expr>> &keys,
                          u32 width, u32 height) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  DS_ID id = "cms_" + std::to_string(width) + "x" + std::to_string(height) + "_" + std::to_string(obj);
  const TNAProperties &properties = tofino_ctx->get_tna().get_properties();

  std::vector<bits_t> keys_sizes;
  for (klee::ref<klee::Expr> key : keys) {
    keys_sizes.push_back(key->getWidth());
  }

  CountMinSketch *cms = new CountMinSketch(properties, id, keys_sizes, width, height);

  if (!tofino_ctx->check_placement(ep, node, cms)) {
    delete cms;
    cms = nullptr;
  }

  return cms;
}

CountMinSketch *reuse_cms(const EP *ep, const Node *node, addr_t obj) {
  const TofinoContext *tofino_ctx = ep->get_ctx().get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::COUNT_MIN_SKETCH && "Invalid DS type");

  CountMinSketch *cms = dynamic_cast<CountMinSketch *>(*ds.begin());

  if (!tofino_ctx->check_placement(ep, node, cms)) {
    return nullptr;
  }

  return cms;
}

} // namespace

TofinoContext *TofinoModuleFactory::get_mutable_tofino_ctx(EP *ep) {
  Context &ctx = ep->get_mutable_ctx();
  return ctx.get_mutable_target_ctx<TofinoContext>();
}

const TofinoContext *TofinoModuleFactory::get_tofino_ctx(const EP *ep) {
  const Context &ctx = ep->get_ctx();
  return ctx.get_target_ctx<TofinoContext>();
}

TNA &TofinoModuleFactory::get_mutable_tna(EP *ep) {
  TofinoContext *ctx = get_mutable_tofino_ctx(ep);
  return ctx->get_mutable_tna();
}

const TNA &TofinoModuleFactory::get_tna(const EP *ep) {
  const TofinoContext *ctx = get_tofino_ctx(ep);
  return ctx->get_tna();
}

bool TofinoModuleFactory::expr_fits_in_action(klee::ref<klee::Expr> expr) {
  ActionExprCompatibilityChecker checker;
  checker.visit(expr);
  return checker.is_compatible();
}

Table *TofinoModuleFactory::build_table(const EP *ep, const Node *node, const table_data_t &data) {
  std::vector<bits_t> keys_size;
  for (klee::ref<klee::Expr> key : data.keys) {
    keys_size.push_back(key->getWidth());
  }

  std::vector<bits_t> params_size;
  for (klee::ref<klee::Expr> value : data.values) {
    params_size.push_back(value->getWidth());
  }

  DS_ID id = "table_" + std::to_string(data.obj) + "_" + std::to_string(node->get_id());

  Table *table = new Table(id, data.num_entries, keys_size, params_size);

  const TofinoContext *tofino_ctx = get_tofino_ctx(ep);
  if (!tofino_ctx->check_placement(ep, node, table)) {
    delete table;
    return nullptr;
  }

  return table;
}

bool TofinoModuleFactory::can_build_table(const EP *ep, const Node *node, const table_data_t &data) {
  Table *table = build_table(ep, node, data);

  if (!table) {
    return false;
  }

  delete table;
  return true;
}

std::unordered_set<Register *>
TofinoModuleFactory::build_or_reuse_vector_registers(const EP *ep, const Node *node,
                                                     const vector_register_data_t &data) {
  std::unordered_set<Register *> regs;

  const Context &ctx       = ep->get_ctx();
  bool regs_already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (regs_already_placed) {
    regs = get_vector_registers(ep, node, data);
  } else {
    regs = build_vector_registers(ep, node, data);
  }

  return regs;
}

bool TofinoModuleFactory::can_build_or_reuse_vector_registers(const EP *ep, const Node *node,
                                                              const vector_register_data_t &data) {
  const Context &ctx       = ep->get_ctx();
  bool regs_already_placed = ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister);

  if (regs_already_placed) {
    return !get_vector_registers(ep, node, data).empty();
  }

  std::unordered_set<Register *> regs = build_vector_registers(ep, node, data);
  bool success                        = !regs.empty();

  for (DS *reg : regs) {
    delete reg;
  }

  return success;
}

FCFSCachedTable *TofinoModuleFactory::build_or_reuse_fcfs_cached_table(const EP *ep, const Node *node, addr_t obj,
                                                                       klee::ref<klee::Expr> key, u32 num_entries,
                                                                       u32 cache_capacity) {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = reuse_fcfs_cached_table(ep, node, obj);
  } else {
    cached_table = build_fcfs_cached_table(ep, node, obj, key, num_entries, cache_capacity);
  }

  return cached_table;
}

FCFSCachedTable *TofinoModuleFactory::get_fcfs_cached_table(const EP *ep, const Node *node, addr_t obj) {
  const Context &ctx              = ep->get_ctx();
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();

  if (!tofino_ctx->has_ds(obj)) {
    return nullptr;
  }

  const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

  assert(ds.size() == 1 && "Invalid number of DS");
  assert((*ds.begin())->type == DSType::FCFS_CACHED_TABLE && "Invalid DS type");

  FCFSCachedTable *cached_table = dynamic_cast<FCFSCachedTable *>(*ds.begin());

  if (!tofino_ctx->check_placement(ep, node, cached_table)) {
    cached_table = nullptr;
  }

  return cached_table;
}

bool TofinoModuleFactory::can_get_or_build_fcfs_cached_table(const EP *ep, const Node *node, addr_t obj,
                                                             klee::ref<klee::Expr> key, u32 num_entries,
                                                             u32 cache_capacity) {
  FCFSCachedTable *cached_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable);

  if (already_placed) {
    cached_table = get_fcfs_cached_table(ep, node, obj);

    if (!cached_table || cached_table->cache_capacity != cache_capacity) {
      return false;
    }

    return true;
  }

  cached_table = build_fcfs_cached_table(ep, node, obj, key, num_entries, cache_capacity);

  if (!cached_table) {
    return false;
  }

  delete cached_table;
  return true;
}

Symbols TofinoModuleFactory::get_dataplane_state(const EP *ep, const Node *node) {
  const node_ids_t &roots = ep->get_target_roots(TargetType::Tofino);
  return node->get_prev_symbols(roots);
}

std::vector<u32> TofinoModuleFactory::enum_fcfs_cache_cap(u32 num_entries) {
  std::vector<u32> capacities;

  u32 cache_capacity = 8;
  while (cache_capacity < num_entries) {
    capacities.push_back(cache_capacity);
    cache_capacity *= 2;

    // Overflow check
    assert((capacities.empty() || capacities.back() < cache_capacity) && "Overflow");
  }

  return capacities;
}

hit_rate_t TofinoModuleFactory::get_fcfs_cache_success_rate(const Context &ctx, const Node *node,
                                                            klee::ref<klee::Expr> key, u32 cache_capacity) {
  constraints_t constraints = node->get_ordered_branch_constraints();
  FlowStats flow_stats      = ctx.get_profiler().get_flow_stats(constraints, key);

  u64 avg_pkts_per_flow = flow_stats.pkts / flow_stats.flows;
  u64 cached_packets    = std::min(flow_stats.pkts, avg_pkts_per_flow * cache_capacity);
  hit_rate_t hit_rate   = flow_stats.pkts == 0 ? 0 : cached_packets / static_cast<hit_rate_t>(flow_stats.pkts);

  // std::cerr << "node: " << node->dump(true, true) << "\n";
  // std::cerr << "avg_pkts_per_flow: " << avg_pkts_per_flow << std::endl;
  // std::cerr << "cached_packets: " << cached_packets << std::endl;
  // std::cerr << "hit_rate: " << hit_rate << std::endl;
  // dbg_pause();

  return hit_rate;
}

HHTable *TofinoModuleFactory::build_or_reuse_hh_table(const EP *ep, const Node *node, addr_t obj,
                                                      const std::vector<klee::ref<klee::Expr>> &keys, u32 num_entries,
                                                      u32 cms_width, u32 cms_height) {
  HHTable *hh_table = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    hh_table = reuse_hh_table(ep, node, obj);
  } else {
    hh_table = build_hh_table(ep, node, obj, keys, num_entries, cms_width, cms_height);
  }

  return hh_table;
}

bool TofinoModuleFactory::can_build_or_reuse_hh_table(const EP *ep, const Node *node, addr_t obj,
                                                      const std::vector<klee::ref<klee::Expr>> &keys, u32 num_entries,
                                                      u32 cms_width, u32 cms_height) {
  HHTable *hh_table = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable);

  if (already_placed) {
    const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>();
    const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

    assert(ds.size() == 1 && "Invalid number of DS");
    assert((*ds.begin())->type == DSType::HH_TABLE && "Invalid DS type");

    hh_table = dynamic_cast<HHTable *>(*ds.begin());

    if (!tofino_ctx->check_placement(ep, node, hh_table)) {
      hh_table = nullptr;
      return false;
    }

    if (hh_table->cms_width != cms_width || hh_table->cms_height != cms_height) {
      return false;
    }

    return true;
  }

  hh_table = build_hh_table(ep, node, obj, keys, num_entries, cms_width, cms_height);

  if (!hh_table) {
    return false;
  }

  delete hh_table;
  return true;
}

hit_rate_t TofinoModuleFactory::get_hh_table_hit_success_rate(const Context &ctx, const Node *node,
                                                              klee::ref<klee::Expr> key, u32 capacity) {
  constraints_t constraints = node->get_ordered_branch_constraints();
  FlowStats flow_stats      = ctx.get_profiler().get_flow_stats(constraints, key);

  u64 top_k = 0;
  for (size_t k = 0; k <= capacity && k < flow_stats.pkts_per_flow.size(); k++) {
    top_k += flow_stats.pkts_per_flow[k];
  }

  assert(top_k <= flow_stats.pkts && "Invalid top_k");
  hit_rate_t hit_rate = top_k / static_cast<hit_rate_t>(flow_stats.pkts);

  return hit_rate;
}

bool TofinoModuleFactory::can_build_or_reuse_cms(const EP *ep, const Node *node, addr_t obj,
                                                 const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
                                                 u32 height) {
  CountMinSketch *cms = nullptr;

  const Context &ctx  = ep->get_ctx();
  bool already_placed = ctx.check_ds_impl(obj, DSImpl::Tofino_CountMinSketch);

  if (already_placed) {
    const TofinoContext *tofino_ctx    = ctx.get_target_ctx<TofinoContext>();
    const std::unordered_set<DS *> &ds = tofino_ctx->get_ds(obj);

    assert(ds.size() == 1 && "Invalid number of DS");
    assert((*ds.begin())->type == DSType::COUNT_MIN_SKETCH && "Invalid DS type");

    cms = dynamic_cast<CountMinSketch *>(*ds.begin());

    if (!tofino_ctx->check_placement(ep, node, cms)) {
      cms = nullptr;
      return false;
    }

    if (cms->width != width || cms->height != height) {
      return false;
    }

    return true;
  }

  cms = build_cms(ep, node, obj, keys, width, height);

  if (!cms) {
    return false;
  }

  delete cms;
  return true;
}

CountMinSketch *TofinoModuleFactory::build_or_reuse_cms(const EP *ep, const Node *node, addr_t obj,
                                                        const std::vector<klee::ref<klee::Expr>> &keys, u32 width,
                                                        u32 height) {
  CountMinSketch *cms = nullptr;

  if (ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_CountMinSketch)) {
    cms = reuse_cms(ep, node, obj);
  } else {
    cms = build_cms(ep, node, obj, keys, width, height);
  }

  return cms;
}

} // namespace tofino
} // namespace synapse