#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableRead.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

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

void get_data(const Context &ctx, const LibBDD::Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys, klee::ref<klee::Expr> &value,
              std::optional<LibCore::symbol_t> &hit) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_get" && "Not a map_get call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  LibCore::symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");

  obj   = LibCore::expr_addr_to_obj_addr(map_addr_expr);
  keys  = Table::build_keys(key, ctx.get_expr_structs());
  value = value_out;
  hit   = map_has_this_key;
}
} // namespace

std::optional<spec_impl_t> DataplaneFCFSCachedTableReadFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_get") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  addr_t map_addr                     = LibCore::expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.can_impl_ds(map_addr, DSImpl::Tofino_FCFSCachedTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneFCFSCachedTableReadFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                      LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<LibCore::symbol_t> found;
  get_data(ep->get_ctx(), call_node, obj, keys, value, found);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const DS_ID id = get_cached_table_id(ep->get_ctx(), obj);

  Module *module  = new DataplaneFCFSCachedTableRead(node, id, obj, keys, value, found);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedTableReadFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<LibCore::symbol_t> found;
  get_data(ctx, call_node, obj, keys, value, found);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<Tofino::TofinoContext>()->get_data_structures().get_ds(obj);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const Tofino::FCFSCachedTable *fcfs_cached_table = dynamic_cast<const Tofino::FCFSCachedTable *>(*ds.begin());

  return std::make_unique<DataplaneFCFSCachedTableRead>(node, fcfs_cached_table->id, obj, keys, value, found);
}

} // namespace Controller
} // namespace LibSynapse