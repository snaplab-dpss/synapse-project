#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableDelete.h>
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
  std::vector<klee::ref<klee::Expr>> keys;
};

map_table_data_t get_table_delete_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_erase" && "Not a map_erase call");

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const map_table_data_t data = {
      .obj  = expr_addr_to_obj_addr(map_addr_expr),
      .keys = Table::build_keys(key, ctx.get_expr_structs()),
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneGuardedMapTableDeleteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  const map_table_data_t data = get_table_delete_data(ctx, map_erase);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_GuardedMapTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneGuardedMapTableDeleteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  const map_table_data_t data = get_table_delete_data(ep->get_ctx(), map_erase);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_GuardedMapTable)) {
    return {};
  }

  Module *module  = new DataplaneGuardedMapTableDelete(ep->get_placement(node->get_id()), node, data.obj, data.keys);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneGuardedMapTableDeleteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_erase = dynamic_cast<const Call *>(node);
  const call_t &call    = map_erase->get_call();

  if (call.function_name != "map_erase") {
    return {};
  }

  const map_table_data_t data = get_table_delete_data(ctx, map_erase);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_GuardedMapTable)) {
    return {};
  }

  return std::make_unique<DataplaneGuardedMapTableDelete>(get_type().instance_id, node, data.obj, data.keys);
}

} // namespace Controller
} // namespace LibSynapse