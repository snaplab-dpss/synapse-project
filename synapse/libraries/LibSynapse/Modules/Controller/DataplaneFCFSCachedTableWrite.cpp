#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableWrite.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::DS_ID;
using Tofino::Table;

namespace {

DS_ID get_fcfs_cached_table_id(const Context &ctx, addr_t obj) {
  const Tofino::TofinoContext *tofino_ctx                 = ctx.get_target_ctx<Tofino::TofinoContext>();
  const std::unordered_set<Tofino::DS *> &data_structures = tofino_ctx->get_data_structures().get_ds(obj);
  assert(data_structures.size() == 1 && "Multiple data structures found");
  Tofino::DS *ds = *data_structures.begin();
  assert(ds->type == Tofino::DSType::FCFSCachedTable && "Not a FCFS cached table");
  return ds->id;
}

void get_data(const Context &ctx, const Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_put" && "Not a map_put call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  obj  = expr_addr_to_obj_addr(map_addr_expr);
  keys = Table::build_keys(key, ctx.get_expr_structs());
}
} // namespace

std::optional<spec_impl_t> DataplaneFCFSCachedTableWriteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return {};
  }

  assert(!future_map_puts.empty() && "No future map puts");

  map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable) || !ctx.check_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  spec_impl_t spec_impl(decide(ep, node), ctx);

  for (const BDDNode *op : future_map_puts) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneFCFSCachedTableWriteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!ep->get_bdd()->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return {};
  }

  assert(!future_map_puts.empty() && "No future map puts");

  map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  get_data(ep->get_ctx(), future_map_puts[0], obj, keys);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const DS_ID id = get_fcfs_cached_table_id(ep->get_ctx(), obj);

  Module *module  = new DataplaneFCFSCachedTableWrite(node, id, obj, keys);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedTableWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!bdd->is_map_update_with_dchain(dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return {};
  }

  assert(!future_map_puts.empty() && "No future map puts");

  map_coalescing_objs_t map_objs;
  if (!bdd->get_map_coalescing_objs_from_dchain_op(dchain_allocate_new_index, map_objs)) {
    return {};
  }

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) || !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  get_data(ctx, future_map_puts[0], obj, keys);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<Tofino::TofinoContext>()->get_data_structures().get_ds(obj);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const Tofino::FCFSCachedTable *fcfs_cached_table = dynamic_cast<const Tofino::FCFSCachedTable *>(*ds.begin());

  return std::make_unique<DataplaneFCFSCachedTableWrite>(node, fcfs_cached_table->id, obj, keys);
}

} // namespace Controller
} // namespace LibSynapse
