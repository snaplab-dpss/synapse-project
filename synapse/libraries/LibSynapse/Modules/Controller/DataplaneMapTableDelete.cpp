#include <LibSynapse/Modules/Controller/DataplaneMapTableDelete.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {

struct map_table_data_t {
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
};

map_table_data_t get_table_delete_data(const Context &ctx, const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_erase" && "Not a map_erase call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const map_table_data_t data = {
      .obj  = LibCore::expr_addr_to_obj_addr(map_addr_expr),
      .keys = Table::build_keys(key, ctx.get_expr_structs()),
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneMapTableDeleteFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *map_erase = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return std::nullopt;
  }

  map_table_data_t data = get_table_delete_data(ctx, map_erase);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_MapTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneMapTableDeleteFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                 LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *map_erase = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return impls;
  }

  map_table_data_t data = get_table_delete_data(ep->get_ctx(), map_erase);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return impls;
  }

  Module *module  = new DataplaneMapTableDelete(node, data.obj, data.keys);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DataplaneMapTableDeleteFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *map_erase = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  map_table_data_t data = get_table_delete_data(ctx, map_erase);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  return std::make_unique<DataplaneMapTableDelete>(node, data.obj, data.keys);
}

} // namespace Controller
} // namespace LibSynapse