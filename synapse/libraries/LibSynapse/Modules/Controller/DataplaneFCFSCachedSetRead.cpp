#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedSetRead.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

void get_data(const Context &ctx, const Call *call_node, addr_t &obj, klee::ref<klee::Expr> &key, symbol_t &hit) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_get" && "Not a map_get call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;

  obj = expr_addr_to_obj_addr(map_addr_expr);
  key = call.args.at("key").in;
  hit = call_node->get_local_symbol("map_has_this_key");
}

} // namespace

std::optional<spec_impl_t> DataplaneFCFSCachedSetReadFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  const addr_t map_addr               = expr_addr_to_obj_addr(map_addr_expr);

  if (!speculations.ctx.can_impl_ds(map_addr, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DataplaneFCFSCachedSetReadFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  addr_t obj;
  klee::ref<klee::Expr> key;
  symbol_t found;
  get_data(ep->get_ctx(), call_node, obj, key, found);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  Module *module  = new DataplaneFCFSCachedSetRead(node, obj, key, found);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedSetReadFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  addr_t obj;
  klee::ref<klee::Expr> key;
  symbol_t found;
  get_data(ctx, call_node, obj, key, found);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  return std::make_unique<DataplaneFCFSCachedSetRead>(node, obj, key, found);
}

} // namespace Controller
} // namespace LibSynapse