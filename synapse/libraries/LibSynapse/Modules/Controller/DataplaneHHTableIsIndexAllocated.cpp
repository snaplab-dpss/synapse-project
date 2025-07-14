#include <LibSynapse/Modules/Controller/DataplaneHHTableIsIndexAllocated.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::Table;

namespace {

struct table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> value;

  table_data_t(const Context &ctx, const Call *map_put) {
    const call_t &call = map_put->get_call();
    assert(call.function_name == "map_put" && "Not a map_put call");

    obj        = expr_addr_to_obj_addr(call.args.at("map").expr);
    key        = call.args.at("key").in;
    table_keys = Table::build_keys(key, ctx.get_expr_structs());
    value      = call.args.at("value").expr;
  }
};

} // namespace

std::optional<spec_impl_t> DataplaneHHTableIsIndexAllocatedFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_is_index_allocated = dynamic_cast<const Call *>(node);
  const call_t &call                    = dchain_is_index_allocated->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return {};
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
  const addr_t obj               = expr_addr_to_obj_addr(obj_expr);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneHHTableIsIndexAllocatedFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_is_index_allocated = dynamic_cast<const Call *>(node);
  const call_t &call                    = dchain_is_index_allocated->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return {};
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index    = call.args.at("index").expr;

  const addr_t obj = expr_addr_to_obj_addr(obj_expr);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  const symbol_t is_allocated = dchain_is_index_allocated->get_local_symbol("is_index_allocated");

  Module *module  = new DataplaneHHTableIsIndexAllocated(node, obj, index, is_allocated);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneHHTableIsIndexAllocatedFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_is_index_allocated = dynamic_cast<const Call *>(node);
  const call_t &call                    = dchain_is_index_allocated->get_call();

  if (call.function_name != "dchain_is_index_allocated") {
    return {};
  }

  klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index    = call.args.at("index").expr;

  const addr_t obj = expr_addr_to_obj_addr(obj_expr);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  const symbol_t is_allocated = dchain_is_index_allocated->get_local_symbol("is_index_allocated");

  return std::make_unique<DataplaneHHTableIsIndexAllocated>(node, obj, index, is_allocated);
}

} // namespace Controller
} // namespace LibSynapse