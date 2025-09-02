#include <LibSynapse/Modules/Tofino/GuardedMapTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

map_table_data_t get_guarded_map_table_data(const Context &ctx, const Call *call_node) {
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
      .time_aware   = ctx.get_map_coalescing_objs(obj).has_value() ? TimeAware::Yes : TimeAware::No,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> GuardedMapTableLookupFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_table_data_t data = get_guarded_map_table_data(ep->get_ctx(), map_get);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_GuardedMapTable)) {
    return {};
  }

  if (!can_build_or_reuse_guarded_map_table(ep, node, target, data)) {
    return {};
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(data.obj, DSImpl::Tofino_GuardedMapTable);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> GuardedMapTableLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_table_data_t data = get_guarded_map_table_data(ep->get_ctx(), map_get);

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Tofino_GuardedMapTable)) {
    return {};
  }

  GuardedMapTable *guarded_map_table = build_or_reuse_guarded_map_table(ep, node, target, data);

  if (!guarded_map_table) {
    return {};
  }

  Module *module  = new GuardedMapTableLookup(ep->get_placement(node->get_id()), node, guarded_map_table->id, data.obj, data.original_key, data.keys,
                                              data.value, data.hit);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(data.obj, DSImpl::Tofino_GuardedMapTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get(), target);
  tofino_ctx->place(new_ep.get(), node, data.obj, guarded_map_table);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> GuardedMapTableLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_table_data_t data = get_guarded_map_table_data(ctx, map_get);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_GuardedMapTable)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>(target)->get_data_structures().get_ds(data.obj);

  const GuardedMapTable *guarded_map_table = dynamic_cast<const GuardedMapTable *>(*ds.begin());

  return std::make_unique<GuardedMapTableLookup>(get_type().instance_id, node, guarded_map_table->id, data.obj, data.original_key, data.keys,
                                                 data.value, data.hit);
}

} // namespace Tofino
} // namespace LibSynapse