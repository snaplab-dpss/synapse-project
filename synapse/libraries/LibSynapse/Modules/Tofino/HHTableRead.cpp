#include <LibSynapse/Modules/Tofino/HHTableRead.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Visualizers/ProfilerVisualizer.h>

namespace LibSynapse {
namespace Tofino {
namespace {

using LibBDD::branch_direction_t;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

struct hh_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> read_value;
  symbol_t map_has_this_key;
  u32 capacity;

  hh_table_data_t(const Context &ctx, const Call *map_get) {
    const call_t &call = map_get->get_call();
    assert(call.function_name == "map_get" && "Not a map_get call");

    const symbol_t map_has_this_key_symbol = map_get->get_local_symbol("map_has_this_key");

    obj              = expr_addr_to_obj_addr(call.args.at("map").expr);
    key              = call.args.at("key").in;
    table_keys       = Table::build_keys(key, ctx.get_expr_structs());
    read_value       = call.args.at("value_out").out;
    map_has_this_key = map_has_this_key_symbol;
    capacity         = ctx.get_map_config(obj).capacity;
  }
};

bool update_map_get_success_hit_rate(const EP *ep, Context &ctx, const BDDNode *map_get, addr_t map, klee::ref<klee::Expr> key, u32 capacity,
                                     u32 cms_width, const branch_direction_t &mgsc) {
  const hit_rate_t success_rate = TofinoModuleFactory::get_hh_table_hit_success_rate(ep, ctx, map_get, map, key, capacity, cms_width);

  assert(mgsc.branch && "No branch checking map_get success");
  const BDDNode *on_success = mgsc.direction ? mgsc.branch->get_on_true() : mgsc.branch->get_on_false();

  if (!ctx.get_profiler().can_set(on_success->get_ordered_branch_constraints(), success_rate)) {
    return false;
  }

  ctx.get_mutable_profiler().set_relative(on_success->get_ordered_branch_constraints(), success_rate);

  return true;
}

} // namespace

std::optional<spec_impl_t> HHTableReadFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const hh_table_data_t table_data(ep->get_ctx(), map_get);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.can_impl_ds(map_objs->map, DSImpl::Tofino_HeavyHitterTable) || !ctx.can_impl_ds(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  const std::vector<hit_rate_t> &failing_to_allocate_new_index_hit_rates = ctx.get_failing_to_allocate_new_index_hit_rates(map_objs->dchain);
  if (std::all_of(failing_to_allocate_new_index_hit_rates.begin(), failing_to_allocate_new_index_hit_rates.end(),
                  [](hit_rate_t index_allocation_failure_hr) { return index_allocation_failure_hr == 0; })) {
    return {};
  }

  const branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return {};
  }

  u32 chosen_cms_width           = 0;
  hit_rate_t chosen_success_rate = 0_hr;

  for (const u32 cms_width : HHTable::CMS_WIDTH_CANDIDATES) {
    const hit_rate_t success_rate =
        TofinoModuleFactory::get_hh_table_hit_success_rate(ep, ctx, map_get, table_data.obj, table_data.key, table_data.capacity, cms_width);

    if (!can_build_or_reuse_hh_table(ep, node, table_data.obj, table_data.table_keys, table_data.capacity, cms_width, HHTable::CMS_HEIGHT)) {
      continue;
    }

    if (success_rate > chosen_success_rate) {
      chosen_success_rate = success_rate;
      chosen_cms_width    = cms_width;
    }
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable);
  new_ctx.save_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable);

  if (!update_map_get_success_hit_rate(ep, new_ctx, map_get, table_data.obj, table_data.key, table_data.capacity, chosen_cms_width, mpsc)) {
    return {};
  }

  return spec_impl_t(decide(ep, node, {{HHTABLE_CMS_WIDTH_PARAM, static_cast<i32>(chosen_cms_width)}}), new_ctx);
}

std::vector<impl_t> HHTableReadFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const hh_table_data_t table_data(ep->get_ctx(), map_get);

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(map_objs->map, DSImpl::Tofino_HeavyHitterTable) ||
      !ep->get_ctx().can_impl_ds(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  const std::vector<hit_rate_t> &failing_to_allocate_new_index_hit_rates =
      ep->get_ctx().get_failing_to_allocate_new_index_hit_rates(map_objs->dchain);
  if (std::all_of(failing_to_allocate_new_index_hit_rates.begin(), failing_to_allocate_new_index_hit_rates.end(),
                  [](hit_rate_t index_allocation_failure_hr) { return index_allocation_failure_hr == 0; })) {
    return {};
  }

  const branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return {};
  }

  std::vector<impl_t> impls;
  for (const u32 cms_width : HHTable::CMS_WIDTH_CANDIDATES) {
    HHTable *hh_table = build_or_reuse_hh_table(ep, node, table_data.obj, table_data.table_keys, table_data.capacity, cms_width, HHTable::CMS_HEIGHT);
    if (!hh_table) {
      continue;
    }

    std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);
    if (!update_map_get_success_hit_rate(new_ep.get(), new_ep->get_mutable_ctx(), map_get, table_data.obj, table_data.key, table_data.capacity,
                                         cms_width, mpsc)) {
      continue;
    }

    Module *module  = new HHTableRead(node, hh_table->id, table_data.obj, table_data.key, table_data.table_keys, table_data.read_value,
                                      table_data.map_has_this_key);
    EPNode *ep_node = new EPNode(module);

    new_ep->get_mutable_ctx().save_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable);
    new_ep->get_mutable_ctx().save_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
    tofino_ctx->place(new_ep.get(), node, map_objs->map, hh_table);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    impls.emplace_back(implement(ep, node, std::move(new_ep), {{HHTABLE_CMS_WIDTH_PARAM, static_cast<i32>(cms_width)}}));
  }

  return impls;
}

std::unique_ptr<Module> HHTableReadFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const hh_table_data_t table_data(ctx, map_get);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable) || !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  const std::vector<hit_rate_t> &failing_to_allocate_new_index_hit_rates = ctx.get_failing_to_allocate_new_index_hit_rates(map_objs->dchain);
  if (std::all_of(failing_to_allocate_new_index_hit_rates.begin(), failing_to_allocate_new_index_hit_rates.end(),
                  [](hit_rate_t index_allocation_failure_hr) { return index_allocation_failure_hr == 0; })) {
    return {};
  }

  const branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(map_objs->map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const HHTable *hh_table = dynamic_cast<const HHTable *>(*ds.begin());

  return std::make_unique<HHTableRead>(node, hh_table->id, table_data.obj, table_data.key, table_data.table_keys, table_data.read_value,
                                       table_data.map_has_this_key);
}

} // namespace Tofino
} // namespace LibSynapse