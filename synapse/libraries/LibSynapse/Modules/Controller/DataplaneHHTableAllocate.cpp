#include <LibSynapse/Modules/Controller/DataplaneHHTableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

using Tofino::DS;
using Tofino::DS_ID;
using Tofino::DSType;
using Tofino::HHTable;
using Tofino::Table;
using Tofino::TofinoContext;

namespace {

struct hh_table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;
};

hh_table_allocation_data_t get_hh_table_allocation_data(const Call *map_allocate) {
  const call_t &call = map_allocate->get_call();
  assert(call.function_name == "map_allocate");

  klee::ref<klee::Expr> key_size   = call.args.at("key_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> map_out    = call.args.at("map_out").out;
  klee::ref<klee::Expr> value_size = solver_toolbox.exprBuilder->Constant(0, klee::Expr::Int32);

  const addr_t obj = expr_addr_to_obj_addr(map_out);

  return {obj, key_size, value_size, capacity};
}

} // namespace

std::optional<spec_impl_t> DataplaneHHTableAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_allocate = dynamic_cast<const Call *>(node);
  const call_t &call       = map_allocate->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  const hh_table_allocation_data_t table_data = get_hh_table_allocation_data(map_allocate);

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneHHTableAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_allocate = dynamic_cast<const Call *>(node);
  const call_t &call       = map_allocate->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  const hh_table_allocation_data_t table_data = get_hh_table_allocation_data(map_allocate);

  const std::optional<map_coalescing_objs_t> map_objs = ep->get_ctx().get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ep->get_ctx().check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  Module *module  = new DataplaneHHTableAllocate(node, table_data.obj, table_data.key_size, table_data.value_size, table_data.capacity);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneHHTableAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_allocate = dynamic_cast<const Call *>(node);
  const call_t &call       = map_allocate->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  const hh_table_allocation_data_t table_data = get_hh_table_allocation_data(map_allocate);

  const std::optional<map_coalescing_objs_t> map_objs = ctx.get_map_coalescing_objs(table_data.obj);
  if (!map_objs.has_value()) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs->map, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  return std::make_unique<DataplaneHHTableAllocate>(node, table_data.obj, table_data.key_size, table_data.value_size, table_data.capacity);
}

} // namespace Controller
} // namespace LibSynapse