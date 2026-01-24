#include <LibSynapse/Modules/Controller/DataplaneMapSetTableDelete.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct map_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
};

map_table_data_t get_table_delete_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_erase" && "Not a map_erase call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const map_table_data_t data = {
      .obj = expr_addr_to_obj_addr(map_addr_expr),
      .key = key,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneMapSetTableDeleteFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  const map_table_data_t data = get_table_delete_data(speculations.ctx, map_erase);

  if (!speculations.ctx.can_impl_ds(data.obj, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DataplaneMapSetTableDeleteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  const map_table_data_t data = get_table_delete_data(ep->get_ctx(), map_erase);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  Module *module  = new DataplaneMapSetTableDelete(node, data.obj, data.key);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneMapSetTableDeleteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  const map_table_data_t data = get_table_delete_data(ctx, map_erase);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  return std::make_unique<DataplaneMapSetTableDelete>(node, data.obj, data.key);
}

} // namespace Controller
} // namespace LibSynapse