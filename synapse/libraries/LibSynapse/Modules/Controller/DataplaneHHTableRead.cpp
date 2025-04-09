#include <LibSynapse/Modules/Controller/DataplaneHHTableRead.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

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
  LibCore::symbol_t map_has_this_key;
  int capacity;

  table_data_t(const Context &ctx, const LibBDD::Call *map_get) {
    const LibBDD::call_t &call = map_get->get_call();
    assert(call.function_name == "map_get" && "Not a map_get call");

    obj              = LibCore::expr_addr_to_obj_addr(call.args.at("map").expr);
    key              = call.args.at("key").in;
    read_value       = call.args.at("value_out").out;
    map_has_this_key = map_get->get_local_symbol("map_has_this_key");
    capacity         = ctx.get_map_config(obj).capacity;
  }
};
} // namespace

std::optional<spec_impl_t> DataplaneHHTableReadFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return std::nullopt;
  }

  const table_data_t table_data(ep->get_ctx(), map_get);

  const std::optional<LibBDD::map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return std::nullopt;
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable) || !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneHHTableReadFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return impls;
  }

  const table_data_t table_data(ep->get_ctx(), map_get);

  const std::optional<LibBDD::map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return impls;
  }

  if (!ep->get_ctx().check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable) ||
      !ep->get_ctx().check_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return impls;
  }

  Module *module  = new DataplaneHHTableRead(node, table_data.obj, table_data.key, table_data.read_value, table_data.map_has_this_key);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DataplaneHHTableReadFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  const table_data_t table_data(ctx, map_get);

  const std::optional<LibBDD::map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable) || !ctx.check_ds_impl(map_objs->dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return std::make_unique<DataplaneHHTableRead>(node, table_data.obj, table_data.key, table_data.read_value, table_data.map_has_this_key);
}

} // namespace Controller
} // namespace LibSynapse