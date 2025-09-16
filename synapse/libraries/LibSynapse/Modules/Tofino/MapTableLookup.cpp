#include <LibSynapse/Modules/Tofino/MapTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

map_table_data_t get_map_table_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_get" && "Unexpected function");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  const symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");
  const addr_t obj                = expr_addr_to_obj_addr(map_addr_expr);

  const map_config_t &cfg = ctx.get_map_config(obj);

  const map_table_data_t data = {
      .obj          = expr_addr_to_obj_addr(map_addr_expr),
      .capacity     = static_cast<u32>(cfg.capacity),
      .original_key = key,
      .keys         = Table::build_keys(key, ctx.get_expr_structs()),
      .value        = value_out,
      .hit          = map_has_this_key,
      .time_aware   = TimeAware::No,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> MapTableLookupFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_table_data_t data = get_map_table_data(ep->get_ctx(), map_get);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  if (!can_build_or_reuse_map_table(ep, node, data)) {
    return {};
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(data.obj, DSImpl::Tofino_MapTable);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> MapTableLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_table_data_t data = get_map_table_data(ep->get_ctx(), map_get);

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  MapTable *map_table = build_or_reuse_map_table(ep, node, data);

  if (!map_table) {
    return {};
  }

  Module *module  = new MapTableLookup(node, map_table->id, data.obj, data.original_key, data.keys, data.value, data.hit);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(data.obj, DSImpl::Tofino_MapTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, data.obj, map_table);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> MapTableLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_table_data_t data = get_map_table_data(ctx, map_get);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(data.obj);
  const MapTable *map_table                 = dynamic_cast<const MapTable *>(*ds.begin());

  return std::make_unique<MapTableLookup>(node, map_table->id, data.obj, data.original_key, data.keys, data.value, data.hit);
}

} // namespace Tofino
} // namespace LibSynapse