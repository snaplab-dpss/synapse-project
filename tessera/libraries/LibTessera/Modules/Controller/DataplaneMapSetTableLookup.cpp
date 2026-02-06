#include <LibTessera/Modules/Controller/DataplaneMapSetTableLookup.h>
#include <LibTessera/ExecutionPlan.h>

namespace LibTessera {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::MapSetTable;
using Tofino::Table;

namespace {

struct map_set_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::optional<symbol_t> found;
};

map_set_table_data_t table_data_from_map_op(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_get" && "Not a map_get call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");

  const map_set_table_data_t data = {
      .obj   = expr_addr_to_obj_addr(map_addr_expr),
      .key   = key,
      .found = map_has_this_key,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneMapSetTableLookupFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_set_table_data_t data = table_data_from_map_op(speculations.ctx, map_get);

  if (!speculations.ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DataplaneMapSetTableLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_set_table_data_t data = table_data_from_map_op(ep->get_ctx(), map_get);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  Module *module  = new DataplaneMapSetTableLookup(node, data.obj, data.key, data.found);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneMapSetTableLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_set_table_data_t data = table_data_from_map_op(ctx, map_get);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapSetTable)) {
    return {};
  }

  return std::make_unique<DataplaneMapSetTableLookup>(node, data.obj, data.key, data.found);
}

} // namespace Controller
} // namespace LibTessera