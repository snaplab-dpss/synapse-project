#include <LibSynapse/Modules/Controller/DataplaneVectorTableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

using Tofino::Table;

namespace {

struct vector_table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;
};

vector_table_allocation_data_t get_vector_table_data(const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "vector_allocate");

  klee::ref<klee::Expr> key_size   = solver_toolbox.exprBuilder->Constant(32, klee::Expr::Int32);
  klee::ref<klee::Expr> value_size = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  const addr_t obj = expr_addr_to_obj_addr(vector_out);

  return {obj, key_size, value_size, capacity};
}

} // namespace

std::optional<spec_impl_t> DataplaneVectorTableAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DataplaneVectorTableAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DataplaneVectorTableAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_allocate = dynamic_cast<const Call *>(node);
  const call_t &call          = vector_allocate->get_call();

  if (call.function_name != "vector_allocate") {
    return {};
  }

  const vector_table_allocation_data_t data = get_vector_table_data(vector_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  return std::make_unique<DataplaneVectorTableAllocate>(type, node, data.obj, data.key_size, data.value_size, data.capacity);
}

} // namespace Controller
} // namespace LibSynapse