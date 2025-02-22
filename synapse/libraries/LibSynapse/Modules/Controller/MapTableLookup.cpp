#include <LibSynapse/Modules/Controller/MapTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::MapTable;
using Tofino::Table;

namespace {

struct map_table_data_t {
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<LibCore::symbol_t> found;
};

map_table_data_t table_data_from_map_op(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_get" && "Not a map_get call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value_out     = call.args.at("value_out").out;

  LibCore::symbol_t map_has_this_key = call_node->get_local_symbol("map_has_this_key");

  map_table_data_t data = {
      .obj   = LibCore::expr_addr_to_obj_addr(map_addr_expr),
      .keys  = Table::build_keys(key),
      .value = value_out,
      .found = map_has_this_key,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> MapTableLookupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return std::nullopt;
  }

  map_table_data_t data = table_data_from_map_op(map_get);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_MapTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> MapTableLookupFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                        LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return impls;
  }

  map_table_data_t data = table_data_from_map_op(map_get);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return impls;
  }

  Module *module  = new MapTableLookup(node, data.obj, data.keys, data.value, data.found);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> MapTableLookupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *map_get = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_get->get_call();

  if (call.function_name != "map_get") {
    return {};
  }

  map_table_data_t data = table_data_from_map_op(map_get);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  return std::make_unique<MapTableLookup>(node, data.obj, data.keys, data.value, data.found);
}

} // namespace Controller
} // namespace LibSynapse