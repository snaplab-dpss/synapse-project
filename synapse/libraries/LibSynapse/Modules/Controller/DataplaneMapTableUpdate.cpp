#include <LibSynapse/Modules/Controller/DataplaneMapTableUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {

struct map_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
};

map_table_data_t get_map_table_update_data(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_put" && "Not a map_put call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value         = call.args.at("value").expr;

  map_table_data_t data = {
      .obj   = LibCore::expr_addr_to_obj_addr(map_addr_expr),
      .key   = key,
      .value = value,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneMapTableUpdateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return std::nullopt;
  }

  map_table_data_t data = get_map_table_update_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_MapTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneMapTableUpdateFactory::process_node(const EP *ep, const LibBDD::Node *node,
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

  map_table_data_t data = get_map_table_update_data(call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return impls;
  }

  Module *module  = new DataplaneMapTableUpdate(node, data.obj, data.key, data.value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DataplaneMapTableUpdateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  map_table_data_t data = get_map_table_update_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  return std::make_unique<DataplaneMapTableUpdate>(node, data.obj, data.key, data.value);
}

} // namespace Controller
} // namespace LibSynapse