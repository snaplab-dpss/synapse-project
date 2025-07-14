#include <LibSynapse/Modules/Controller/DataplaneHHTableDelete.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::Table;

namespace {

void get_map_erase_data(const Context &ctx, const Call *call_node, addr_t &obj, std::vector<klee::ref<klee::Expr>> &keys) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_erase" && "Not a map_erase call");

  obj  = expr_addr_to_obj_addr(call.args.at("map").expr);
  keys = Table::build_keys(call.args.at("key").in, ctx.get_expr_structs());
}

} // namespace

std::optional<spec_impl_t> DataplaneHHTableDeleteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  const addr_t map_addr               = expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.can_impl_ds(map_addr, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneHHTableDeleteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
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

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  Module *module  = new DataplaneHHTableDelete(node, obj, keys);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneHHTableDeleteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
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

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return std::make_unique<DataplaneHHTableDelete>(node, obj, keys);
}

} // namespace Controller
} // namespace LibSynapse