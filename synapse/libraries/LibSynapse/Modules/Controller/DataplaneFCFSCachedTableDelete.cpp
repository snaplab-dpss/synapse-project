#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableDelete.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::DS_ID;
using Tofino::Table;

namespace {

DS_ID get_cached_table_id(const Context &ctx, addr_t obj) {
  const Tofino::TofinoContext *tofino_ctx                 = ctx.get_target_ctx<Tofino::TofinoContext>();
  const std::unordered_set<Tofino::DS *> &data_structures = tofino_ctx->get_data_structures().get_ds(obj);
  assert(data_structures.size() == 1 && "Multiple data structures found");
  Tofino::DS *ds = *data_structures.begin();
  assert(ds->type == Tofino::DSType::FCFSCachedTable && "Not a FCFS cached table");
  return ds->id;
}

void get_map_erase_data(const Context &ctx, const Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_erase" && "Not a map_erase call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  obj  = expr_addr_to_obj_addr(map_addr_expr);
  keys = Table::build_keys(key, ctx.get_expr_structs());
}
} // namespace

std::optional<spec_impl_t> DataplaneFCFSCachedTableDeleteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  addr_t map_addr                     = expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.can_impl_ds(map_addr, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneFCFSCachedTableDeleteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  get_map_erase_data(ep->get_ctx(), call_node, obj, keys);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const DS_ID id = get_cached_table_id(ep->get_ctx(), obj);

  Module *module  = new DataplaneFCFSCachedTableDelete(node, id, obj, keys);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedTableDeleteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  get_map_erase_data(ctx, call_node, obj, keys);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<Tofino::TofinoContext>()->get_data_structures().get_ds(obj);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const Tofino::FCFSCachedTable *fcfs_cached_table = dynamic_cast<const Tofino::FCFSCachedTable *>(*ds.begin());

  return std::make_unique<DataplaneFCFSCachedTableDelete>(node, fcfs_cached_table->id, obj, keys);
}

} // namespace Controller
} // namespace LibSynapse