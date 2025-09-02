#include <LibSynapse/Modules/Controller/DataplaneHHTableRead.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::DS;
using Tofino::DS_ID;
using Tofino::DSType;
using Tofino::HHTable;
using Tofino::Table;
using Tofino::TofinoContext;

namespace {
struct table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  symbol_t map_has_this_key;
  int capacity;

  table_data_t(const Context &ctx, const Call *map_get) {
    const call_t &call = map_get->get_call();
    assert(call.function_name == "map_get" && "Not a map_get call");

    obj              = expr_addr_to_obj_addr(call.args.at("map").expr);
    key              = call.args.at("key").in;
    read_value       = call.args.at("value_out").out;
    map_has_this_key = map_get->get_local_symbol("map_has_this_key");
    capacity         = ctx.get_map_config(obj).capacity;
  }
};
} // namespace

std::optional<spec_impl_t> DataplaneHHTableReadFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const table_data_t table_data(ep->get_ctx(), map_get);

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable) || !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneHHTableReadFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const table_data_t table_data(ep->get_ctx(), map_get);

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ep->get_ctx().check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable) ||
      !ep->get_ctx().check_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  Module *module  = new DataplaneHHTableRead(ep->get_placement(node->get_id()), node, table_data.obj, table_data.key, table_data.read_value,
                                             table_data.map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneHHTableReadFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_get = dynamic_cast<const Call *>(node);
  const call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const table_data_t table_data(ctx, map_get);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable) || !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return std::make_unique<DataplaneHHTableRead>(get_type().instance_id, node, table_data.obj, table_data.key, table_data.read_value,
                                                table_data.map_has_this_key);
}

} // namespace Controller
} // namespace LibSynapse