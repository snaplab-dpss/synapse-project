#include <LibSynapse/Modules/Controller/HHTableUpdate.h>
#include <LibSynapse/Modules/Controller/HHTableRead.h>
#include <LibSynapse/Modules/Tofino/HHTableRead.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

namespace {

using Tofino::Table;

struct table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> value;

  table_data_t(const Context &ctx, const LibBDD::Call *map_put) {
    const LibBDD::call_t &call = map_put->get_call();
    assert(call.function_name == "map_put" && "Not a map_put call");

    obj        = LibCore::expr_addr_to_obj_addr(call.args.at("map").expr);
    key        = call.args.at("key").in;
    table_keys = Table::build_keys(key, ctx.get_expr_structs());
    value      = call.args.at("value").expr;
  }
};

} // namespace

std::optional<spec_impl_t> HHTableUpdateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *map_put = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_put->get_call();

  if (call.function_name != "map_put") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
  addr_t obj                     = LibCore::expr_addr_to_obj_addr(obj_expr);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> HHTableUpdateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                       LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *map_put = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_put->get_call();

  if (call.function_name != "map_put") {
    return impls;
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
  const addr_t obj               = LibCore::expr_addr_to_obj_addr(obj_expr);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return impls;
  }

  const table_data_t table_data(ep->get_ctx(), map_put);

  Module *module  = new HHTableUpdate(node, table_data.obj, table_data.table_keys, table_data.value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> HHTableUpdateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *map_put = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call  = map_put->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
  const addr_t obj               = LibCore::expr_addr_to_obj_addr(obj_expr);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  const table_data_t table_data(ctx, map_put);

  return std::make_unique<HHTableUpdate>(node, table_data.obj, table_data.table_keys, table_data.value);
}

} // namespace Controller
} // namespace LibSynapse