#include "hh_table_read.h"

namespace synapse {
namespace tofino {
namespace {

struct hh_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> read_value;
  symbol_t map_has_this_key;
  u32 num_entries;

  hh_table_data_t(const EP *ep, const Call *map_get) {
    const call_t &call = map_get->get_call();
    SYNAPSE_ASSERT(call.function_name == "map_get", "Not a map_get call");

    symbol_t map_has_this_key_symbol = map_get->get_local_symbol("map_has_this_key");

    obj = expr_addr_to_obj_addr(call.args.at("map").expr);
    key = call.args.at("key").in;
    table_keys = Table::build_keys(key);
    read_value = call.args.at("value_out").out;
    map_has_this_key = map_has_this_key_symbol;
    num_entries = ep->get_ctx().get_map_config(obj).capacity;
  }
};

void update_map_get_success_hit_rate(Context &ctx, const Node *map_get, klee::ref<klee::Expr> key,
                                     u32 capacity, const branch_direction_t &mgsc) {
  hit_rate_t success_rate =
      TofinoModuleFactory::get_hh_table_hit_success_rate(ctx, map_get, key, capacity);

  SYNAPSE_ASSERT(mgsc.branch, "No branch checking map_get success");
  const Node *on_success =
      mgsc.direction ? mgsc.branch->get_on_true() : mgsc.branch->get_on_false();
  const Node *on_failure =
      mgsc.direction ? mgsc.branch->get_on_false() : mgsc.branch->get_on_true();

  hit_rate_t branch_hr = ctx.get_profiler().get_hr(mgsc.branch);

  ctx.get_mutable_profiler().set(on_success->get_ordered_branch_constraints(),
                                 branch_hr * success_rate);
  ctx.get_mutable_profiler().set(on_failure->get_ordered_branch_constraints(),
                                 branch_hr * (1 - success_rate));
}
} // namespace

std::optional<spec_impl_t> HHTableReadFactory::speculate(const EP *ep, const Node *node,
                                                         const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call = map_get->get_call();

  if (call.function_name != "map_get") {
    return std::nullopt;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
      !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return std::nullopt;
  }

  hh_table_data_t table_data(ep, map_get);

  if (!can_build_or_reuse_hh_table(ep, node, table_data.obj, table_data.table_keys,
                                   table_data.num_entries, CMS_WIDTH, CMS_HEIGHT)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable);
  new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable);

  update_map_get_success_hit_rate(new_ctx, map_get, table_data.key, table_data.num_entries, mpsc);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> HHTableReadFactory::process_node(const EP *ep, const Node *node,
                                                     SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call = map_get->get_call();

  if (call.function_name != "map_get") {
    return impls;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
      !ep->get_ctx().can_impl_ds(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return impls;
  }

  branch_direction_t mpsc = map_get->get_map_get_success_check();
  if (!mpsc.branch) {
    return impls;
  }

  hh_table_data_t table_data(ep, map_get);

  HHTable *hh_table = build_or_reuse_hh_table(ep, node, table_data.obj, table_data.table_keys,
                                              table_data.num_entries, CMS_WIDTH, CMS_HEIGHT);

  if (!hh_table) {
    return impls;
  }

  symbol_t min_estimate =
      symbol_manager->create_symbol("min_estimate_" + std::to_string(map_get->get_id()), 32);
  Module *module =
      new HHTableRead(node, hh_table->id, table_data.obj, table_data.table_keys,
                      table_data.read_value, table_data.map_has_this_key, min_estimate);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  new_ep->get_mutable_ctx().save_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable);
  new_ep->get_mutable_ctx().save_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, map_objs.map, hh_table);

  update_map_get_success_hit_rate(new_ep->get_mutable_ctx(), map_get, table_data.key,
                                  table_data.num_entries, mpsc);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino
} // namespace synapse