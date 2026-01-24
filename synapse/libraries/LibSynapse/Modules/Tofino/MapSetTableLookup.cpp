#include <LibSynapse/Modules/Tofino/MapSetTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

map_set_table_data_t get_map_set_table_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_get" && "Unexpected function");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  const symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");
  const symbol_t index            = call_node->get_local_symbol("allocated_index");
  const addr_t obj                = expr_addr_to_obj_addr(map_addr_expr);

  const map_config_t &cfg = ctx.get_map_config(obj);

  const map_set_table_data_t data = {
      .obj          = expr_addr_to_obj_addr(map_addr_expr),
      .capacity     = static_cast<u32>(cfg.capacity),
      .original_key = key,
      .keys         = TofinoModuleFactory::partition_expr_for_registers(ctx, data.original_key),
      .index        = index,
      .hit          = map_has_this_key,
  };

  return data;
}

std::unique_ptr<BDD> rebuild_bdd(EP *new_ep, const BDDNode *node, const map_set_table_data_t &map_set_table_data,
                                 const map_coalescing_objs_t &map_coalescing_objs, BDDNode *&new_next) {
  const BDD *old_bdd       = new_ep->get_bdd();
  std::unique_ptr<BDD> bdd = std::make_unique<BDD>(*old_bdd);

  new_next = bdd->get_mutable_node_by_id(node->get_next()->get_id());

  Call *map_get = dynamic_cast<Call *>(bdd->get_mutable_node_by_id(node->get_id()));
  map_get->remove_local_symbol(map_set_table_data.index.name);

  for (const BDDNode *dchain_rejuvenate_index : map_get->get_future_functions({"dchain_rejuvenate_index"})) {
    BDDNode *new_anchor = bdd->delete_non_branch(dchain_rejuvenate_index->get_id());
    if (new_anchor->get_id() == new_next->get_id()) {
      new_next = new_anchor;
    }
  }

  bdd->assert_inspection();

  return bdd;
}

} // namespace

std::optional<spec_impl_t> MapSetTableLookupFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_set_table_data_t data = get_map_set_table_data(ep->get_ctx(), map_get);

  const std::optional<map_coalescing_objs_t> coalescing_objs = speculations.ctx.get_map_coalescing_objs(data.obj);
  if (!coalescing_objs.has_value()) {
    return {};
  }

  if (!coalescing_objs->vectors.empty()) {
    return {};
  }

  if (!speculations.ctx.is_dchain_used_exclusively_for_linking_maps_with_vectors(coalescing_objs->dchain)) {
    return {};
  }

  if (!speculations.ctx.can_impl_ds(coalescing_objs->map, DSImpl::Tofino_MapSetTable) ||
      !speculations.ctx.can_impl_ds(coalescing_objs->dchain, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  if (!can_build_or_reuse_map_set_table(ep, node, data)) {
    return {};
  }

  Context new_ctx = speculations.ctx;
  new_ctx.save_ds_impl(node->get_id(), coalescing_objs->map, DSImpl::Tofino_MapSetTable);
  new_ctx.save_ds_impl(node->get_id(), coalescing_objs->dchain, DSImpl::Tofino_MapSetTable);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> MapSetTableLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_set_table_data_t data = get_map_set_table_data(ep->get_ctx(), map_get);

  const std::optional<map_coalescing_objs_t> coalescing_objs = ep->get_ctx().get_map_coalescing_objs(data.obj);
  if (!coalescing_objs.has_value()) {
    return {};
  }

  if (!coalescing_objs->vectors.empty()) {
    return {};
  }

  if (!ep->get_ctx().is_dchain_used_exclusively_for_linking_maps_with_vectors(coalescing_objs->dchain)) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(coalescing_objs->map, DSImpl::Tofino_MapSetTable) ||
      !ep->get_ctx().can_impl_ds(coalescing_objs->dchain, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  MapSetTable *map_set_table = build_or_reuse_map_set_table(ep, node, data);

  if (!map_set_table) {
    return {};
  }

  Module *module  = new MapSetTableLookup(node, map_set_table->id, data.obj, data.original_key, data.keys, data.hit);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  BDDNode *new_next;
  std::unique_ptr<BDD> new_bdd = rebuild_bdd(new_ep.get(), node, data, *coalescing_objs, new_next);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), coalescing_objs->map, DSImpl::Tofino_MapSetTable);
  ctx.save_ds_impl(node->get_id(), coalescing_objs->dchain, DSImpl::Tofino_MapSetTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, data.obj, map_set_table);

  const EPLeaf leaf(ep_node, new_next);
  new_ep->process_leaf(ep_node, {leaf});
  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> MapSetTableLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_set_table_data_t data = get_map_set_table_data(ctx, map_get);

  const std::optional<map_coalescing_objs_t> coalescing_objs = ctx.get_map_coalescing_objs(data.obj);
  if (!coalescing_objs.has_value()) {
    return {};
  }

  if (!coalescing_objs->vectors.empty()) {
    return {};
  }

  if (!ctx.is_dchain_used_exclusively_for_linking_maps_with_vectors(coalescing_objs->dchain)) {
    return {};
  }

  if (!ctx.check_ds_impl(coalescing_objs->map, DSImpl::Tofino_MapSetTable) ||
      !ctx.check_ds_impl(coalescing_objs->dchain, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(coalescing_objs->map);
  const MapSetTable *map_set_table          = dynamic_cast<const MapSetTable *>(*ds.begin());

  return std::make_unique<MapSetTableLookup>(node, map_set_table->id, data.obj, data.original_key, data.keys, data.hit);
}

} // namespace Tofino
} // namespace LibSynapse