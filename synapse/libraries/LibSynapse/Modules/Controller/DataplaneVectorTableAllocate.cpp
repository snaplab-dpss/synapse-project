#include <LibSynapse/Modules/Controller/DataplaneVectorTableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {

struct vector_table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;
};

vector_table_allocation_data_t get_vector_table_data(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "vector_allocate");

  klee::ref<klee::Expr> key_size   = LibCore::solver_toolbox.exprBuilder->Constant(32, klee::Expr::Int32);
  klee::ref<klee::Expr> value_size = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  addr_t obj = LibCore::expr_addr_to_obj_addr(vector_out);

  return {obj, key_size, value_size, capacity};
}

} // namespace

std::optional<spec_impl_t> DataplaneVectorTableAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DataplaneVectorTableAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                      LibCore::SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DataplaneVectorTableAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *vector_allocate = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call          = vector_allocate->get_call();

  if (call.function_name != "vector_allocate") {
    return {};
  }

  vector_table_allocation_data_t data = get_vector_table_data(vector_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  return std::make_unique<DataplaneVectorTableAllocate>(node, data.obj, data.key_size, data.value_size, data.capacity);
}

} // namespace Controller
} // namespace LibSynapse