#include <LibSynapse/Modules/Controller/DataplaneMapTableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {

struct map_table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;
};

map_table_allocation_data_t get_map_table_data(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_allocate");

  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size   = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out    = call.args.at("map_out").out;
  klee::ref<klee::Expr> value_size = LibCore::solver_toolbox.exprBuilder->Constant(32, klee::Expr::Int32);

  addr_t obj = LibCore::expr_addr_to_obj_addr(map_out);

  return {obj, key_size, value_size, capacity};
}

} // namespace

std::optional<spec_impl_t> DataplaneMapTableAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DataplaneMapTableAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                   LibCore::SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DataplaneMapTableAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *map_allocate = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call       = map_allocate->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  map_table_allocation_data_t data = get_map_table_data(map_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_MapTable)) {
    return {};
  }

  return std::make_unique<DataplaneMapTableAllocate>(node, data.obj, data.key_size, data.value_size, data.capacity);
}

} // namespace Controller
} // namespace LibSynapse