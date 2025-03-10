#include <LibSynapse/Modules/Tofino/MapTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {

map_table_data_t get_map_table_data(const Context &ctx, const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_get" && "Unexpected function");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  LibCore::symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");
  addr_t obj                         = LibCore::expr_addr_to_obj_addr(map_addr_expr);

  const LibBDD::map_config_t &cfg = ctx.get_map_config(obj);

  map_table_data_t data = {
      .obj         = LibCore::expr_addr_to_obj_addr(map_addr_expr),
      .num_entries = static_cast<u32>(cfg.capacity),
      .keys        = Table::build_keys(key),
      .value       = value_out,
      .hit         = map_has_this_key,
      .time_aware  = ctx.get_map_coalescing_objs(obj).has_value() ? TimeAware::Yes : TimeAware::No,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> MapTableLookupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return std::nullopt;
  }

  map_table_data_t data = get_map_table_data(ep->get_ctx(), map_get);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_MapTable)) {
    return std::nullopt;
  }

  if (!can_build_or_reuse_map_table(ep, node, data)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(data.obj, DSImpl::Tofino_MapTable);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> MapTableLookupFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                        LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return impls;
  }

  map_table_data_t data = get_map_table_data(ep->get_ctx(), map_get);

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Tofino_MapTable)) {
    return impls;
  }

  MapTable *map_table = build_or_reuse_map_table(ep, node, data);

  if (!map_table) {
    return impls;
  }

  Module *module  = new MapTableLookup(node, map_table->id, data.obj, data.keys, data.value, data.hit);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(data.obj, DSImpl::Tofino_MapTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, data.obj, map_table);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> MapTableLookupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  map_table_data_t data = get_map_table_data(ctx, map_get);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_ds(data.obj);

  const MapTable *map_table = dynamic_cast<const MapTable *>(*ds.begin());

  return std::make_unique<MapTableLookup>(node, map_table->id, data.obj, data.keys, data.value, data.hit);
}

} // namespace Tofino
} // namespace LibSynapse