#include <LibSynapse/Modules/Controller/DchainTableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

namespace {

struct dchain_table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> capacity;
};

dchain_table_allocation_data_t get_dchain_table_data(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "dchain_allocate");

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;
  klee::ref<klee::Expr> key_size    = LibCore::solver_toolbox.exprBuilder->Constant(32, klee::Expr::Int32);

  dchain_table_allocation_data_t data = {
      .obj      = LibCore::expr_addr_to_obj_addr(chain_out),
      .key_size = key_size,
      .capacity = index_range,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DchainTableAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DchainTableAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                             LibCore::SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DchainTableAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *dchain_allocate = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call          = dchain_allocate->get_call();

  if (call.function_name != "dchain_allocate") {
    return {};
  }

  dchain_table_allocation_data_t data = get_dchain_table_data(dchain_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_DchainTable)) {
    return {};
  }

  return std::make_unique<DchainTableAllocate>(node, data.obj, data.key_size, data.capacity);
}

} // namespace Controller
} // namespace LibSynapse