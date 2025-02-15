#include <LibSynapse/Modules/Controller/TableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {

struct table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;
};

table_allocation_data_t table_data_from_map_op(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_allocate");

  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size   = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out    = call.args.at("map_out").out;
  klee::ref<klee::Expr> value_size = LibCore::solver_toolbox.exprBuilder->Constant(32, klee::Expr::Int32);

  addr_t obj = LibCore::expr_addr_to_obj_addr(map_out);

  return {obj, key_size, value_size, capacity};
}

table_allocation_data_t table_data_from_vector_op(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "vector_allocate");

  klee::ref<klee::Expr> key_size   = LibCore::solver_toolbox.exprBuilder->Constant(32, klee::Expr::Int32);
  klee::ref<klee::Expr> value_size = call.args.at("elem_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> vector_out = call.args.at("vector_out").out;

  addr_t obj = LibCore::expr_addr_to_obj_addr(vector_out);

  return {obj, key_size, value_size, capacity};
}

table_allocation_data_t table_data_from_dchain_op(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "dchain_allocate");

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;
  klee::ref<klee::Expr> key_size    = LibCore::solver_toolbox.exprBuilder->Constant(32, klee::Expr::Int32);
  klee::ref<klee::Expr> value_size  = nullptr;

  addr_t obj = LibCore::expr_addr_to_obj_addr(chain_out);

  return {obj, key_size, value_size, index_range};
}

std::optional<table_allocation_data_t> get_table_lookup_data(const LibBDD::Call *call_node) {
  std::optional<table_allocation_data_t> data;

  const LibBDD::call_t &call = call_node->get_call();

  if (call.function_name == "map_allocate") {
    data = table_data_from_map_op(call_node);
  } else if (call.function_name == "vector_allocate") {
    data = table_data_from_vector_op(call_node);
  } else if (call.function_name == "dchain_allocate") {
    data = table_data_from_dchain_op(call_node);
  }

  return data;
}

} // namespace

std::optional<spec_impl_t> TableAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> TableAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                       LibCore::SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> TableAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);

  std::optional<table_allocation_data_t> data = get_table_lookup_data(call_node);
  if (!data) {
    return {};
  }

  if (!ctx.check_ds_impl(data->obj, DSImpl::Tofino_Table)) {
    return {};
  }

  return std::make_unique<TableAllocate>(node, data->obj, data->key_size, data->value_size, data->capacity);
}

} // namespace Controller
} // namespace LibSynapse