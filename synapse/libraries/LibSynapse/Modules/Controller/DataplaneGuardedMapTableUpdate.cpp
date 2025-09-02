#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::Table;

namespace {

struct map_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
};

map_table_data_t get_map_table_update_data(const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_put" && "Not a map_put call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> value         = call.args.at("value").expr;

  const map_table_data_t data = {
      .obj   = expr_addr_to_obj_addr(map_addr_expr),
      .key   = key,
      .value = value,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneGuardedMapTableUpdateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  const map_table_data_t data = get_map_table_update_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_GuardedMapTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneGuardedMapTableUpdateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  const map_table_data_t data = get_map_table_update_data(call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_GuardedMapTable)) {
    return {};
  }

  Module *module  = new DataplaneGuardedMapTableUpdate(ep->get_placement(node->get_id()), node, data.obj, data.key, data.value);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneGuardedMapTableUpdateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_put") {
    return {};
  }

  const map_table_data_t data = get_map_table_update_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_GuardedMapTable)) {
    return {};
  }

  return std::make_unique<DataplaneGuardedMapTableUpdate>(get_type().instance_id, node, data.obj, data.key, data.value);
}

} // namespace Controller
} // namespace LibSynapse