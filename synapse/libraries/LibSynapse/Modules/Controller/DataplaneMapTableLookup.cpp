#include <LibSynapse/Modules/Controller/DataplaneMapTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::MapTable;
using Tofino::Table;

namespace {

struct map_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  std::optional<symbol_t> found;
};

map_table_data_t table_data_from_map_op(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_get" && "Not a map_get call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  const symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");

  const map_table_data_t data = {
      .obj   = expr_addr_to_obj_addr(map_addr_expr),
      .key   = key,
      .value = value_out,
      .found = map_has_this_key,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneMapTableLookupFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_table_data_t data = table_data_from_map_op(ctx, map_get);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneMapTableLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_table_data_t data = table_data_from_map_op(ep->get_ctx(), map_get);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  Module *module  = new DataplaneMapTableLookup(get_type().instance_id, node, data.obj, data.key, data.value, data.found);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneMapTableLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const map_table_data_t data = table_data_from_map_op(ctx, map_get);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  return std::make_unique<DataplaneMapTableLookup>(get_type().instance_id, node, data.obj, data.key, data.value, data.found);
}

} // namespace Controller
} // namespace LibSynapse