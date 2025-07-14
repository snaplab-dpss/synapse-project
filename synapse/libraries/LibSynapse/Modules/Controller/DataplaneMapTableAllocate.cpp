#include <LibSynapse/Modules/Controller/DataplaneMapTableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

using Tofino::Table;

namespace {

struct map_table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;
};

map_table_allocation_data_t get_map_table_data(const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_allocate");

  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size   = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out    = call.args.at("map_out").out;
  klee::ref<klee::Expr> value_size = solver_toolbox.exprBuilder->Constant(32, klee::Expr::Int32);

  const addr_t obj = expr_addr_to_obj_addr(map_out);

  return {obj, key_size, value_size, capacity};
}

} // namespace

std::optional<spec_impl_t> DataplaneMapTableAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DataplaneMapTableAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DataplaneMapTableAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *map_allocate = dynamic_cast<const Call *>(node);
  const call_t &call       = map_allocate->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  const map_table_allocation_data_t data = get_map_table_data(map_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  return std::make_unique<DataplaneMapTableAllocate>(node, data.obj, data.key_size, data.value_size, data.capacity);
}

} // namespace Controller
} // namespace LibSynapse