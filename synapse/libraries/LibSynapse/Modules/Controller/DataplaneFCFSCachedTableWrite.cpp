#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableWrite.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS_ID;
using Tofino::Table;

namespace {

DS_ID get_cached_table_id(const Context &ctx, addr_t obj) {
  const Tofino::TofinoContext *tofino_ctx                 = ctx.get_target_ctx<Tofino::TofinoContext>();
  const std::unordered_set<Tofino::DS *> &data_structures = tofino_ctx->get_ds(obj);
  assert(data_structures.size() == 1 && "Multiple data structures found");
  Tofino::DS *ds = *data_structures.begin();
  assert(ds->type == Tofino::DSType::FCFSCachedTable && "Not a FCFS cached table");
  return ds->id;
}

void get_data(const Context &ctx, const LibBDD::Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys,
              klee::ref<klee::Expr> &value) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_put" && "Not a map_put call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  value                               = call.args.at("value").expr;

  obj   = LibCore::expr_addr_to_obj_addr(map_addr_expr);
  keys  = Table::build_keys(key, ctx.get_expr_structs());
  value = value;
}
} // namespace

std::optional<spec_impl_t> DataplaneFCFSCachedTableWriteFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  addr_t map_addr                     = LibCore::expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.check_ds_impl(map_addr, DSImpl::Tofino_FCFSCachedTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneFCFSCachedTableWriteFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                       LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return impls;
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  get_data(ep->get_ctx(), call_node, obj, keys, value);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable)) {
    return impls;
  }

  DS_ID id = get_cached_table_id(ep->get_ctx(), obj);

  Module *module  = new DataplaneFCFSCachedTableWrite(node, id, obj, keys, value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedTableWriteFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  get_data(ctx, call_node, obj, keys, value);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<Tofino::TofinoContext>()->get_ds(obj);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const Tofino::FCFSCachedTable *fcfs_cached_table = dynamic_cast<const Tofino::FCFSCachedTable *>(*ds.begin());

  return std::make_unique<DataplaneFCFSCachedTableWrite>(node, fcfs_cached_table->id, obj, keys, value);
}

} // namespace Controller
} // namespace LibSynapse