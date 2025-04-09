#include <LibSynapse/Modules/Tofino/HHTableRead.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Visualizers/ProfilerVisualizer.h>

namespace LibSynapse {
namespace Tofino {
namespace {

struct hh_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> read_value;
  LibCore::symbol_t map_has_this_key;
  u32 capacity;

  hh_table_data_t(const Context &ctx, const LibBDD::Call *map_get) {
    const LibBDD::call_t &call = map_get->get_call();
    assert(call.function_name == "map_get" && "Not a map_get call");

    const LibCore::symbol_t map_has_this_key_symbol = map_get->get_local_symbol("map_has_this_key");

    obj              = LibCore::expr_addr_to_obj_addr(call.args.at("map").expr);
    key              = call.args.at("key").in;
    table_keys       = Table::build_keys(key, ctx.get_expr_structs());
    read_value       = call.args.at("value_out").out;
    map_has_this_key = map_has_this_key_symbol;
    capacity         = ctx.get_map_config(obj).capacity;
  }
};

void update_map_get_success_hit_rate(Context &ctx, const LibBDD::Node *map_get, klee::ref<klee::Expr> key, u32 capacity,
                                     const LibBDD::branch_direction_t &mgsc) {
  const hit_rate_t success_rate = TofinoModuleFactory::get_hh_table_hit_success_rate(ctx, map_get, key, capacity);

  assert(mgsc.branch && "No branch checking map_get success");
  const LibBDD::Node *on_success = mgsc.direction ? mgsc.branch->get_on_true() : mgsc.branch->get_on_false();
  const LibBDD::Node *on_failure = mgsc.direction ? mgsc.branch->get_on_false() : mgsc.branch->get_on_true();

  const hit_rate_t branch_hr      = ctx.get_profiler().get_hr(mgsc.branch);
  const hit_rate_t new_success_hr = hit_rate_t{branch_hr * success_rate};
  const hit_rate_t new_failure_hr = hit_rate_t{branch_hr * (1 - success_rate)};

  ctx.get_mutable_profiler().set(on_success->get_ordered_branch_constraints(), new_success_hr);
  ctx.get_mutable_profiler().set(on_failure->get_ordered_branch_constraints(), new_failure_hr);
}
} // namespace

std::optional<spec_impl_t> HHTableReadFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return std::nullopt;
  }

  const hh_table_data_t table_data(ep->get_ctx(), map_get);

  const std::optional<LibBDD::map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs->map, DSImpl::Tofino_HeavyHitterTable) || !ctx.can_impl_ds(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  const LibBDD::branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return std::nullopt;
  }

  if (!can_build_or_reuse_hh_table(ep, node, table_data.obj, table_data.table_keys, table_data.capacity, CMS_WIDTH, CMS_HEIGHT)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable);
  new_ctx.save_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable);

  update_map_get_success_hit_rate(new_ctx, map_get, table_data.key, table_data.capacity, mpsc);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> HHTableReadFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return impls;
  }

  const hh_table_data_t table_data(ep->get_ctx(), map_get);

  const std::optional<LibBDD::map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs->map, DSImpl::Tofino_HeavyHitterTable) ||
      !ep->get_ctx().can_impl_ds(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return impls;
  }

  LibBDD::branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return impls;
  }

  HHTable *hh_table = build_or_reuse_hh_table(ep, node, table_data.obj, table_data.table_keys, table_data.capacity, CMS_WIDTH, CMS_HEIGHT);

  if (!hh_table) {
    return impls;
  }

  Module *module =
      new HHTableRead(node, hh_table->id, table_data.obj, table_data.key, table_data.table_keys, table_data.read_value, table_data.map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  new_ep->get_mutable_ctx().save_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable);
  new_ep->get_mutable_ctx().save_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, map_objs->map, hh_table);

  update_map_get_success_hit_rate(new_ep->get_mutable_ctx(), map_get, table_data.key, table_data.capacity, mpsc);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> HHTableReadFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const hh_table_data_t table_data(ctx, map_get);

  const std::optional<LibBDD::map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable) || !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  const LibBDD::branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return {};
  }

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_ds(map_objs->map);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const HHTable *hh_table = dynamic_cast<const HHTable *>(*ds.begin());

  return std::make_unique<HHTableRead>(node, hh_table->id, table_data.obj, table_data.key, table_data.table_keys, table_data.read_value,
                                       table_data.map_has_this_key);
}

} // namespace Tofino
} // namespace LibSynapse