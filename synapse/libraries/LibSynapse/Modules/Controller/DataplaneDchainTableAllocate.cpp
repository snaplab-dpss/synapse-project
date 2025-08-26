#include <LibSynapse/Modules/Controller/DataplaneDchainTableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

namespace {

struct dchain_table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> capacity;
  time_ns_t expiration_time;
};

dchain_table_allocation_data_t get_dchain_table_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "dchain_allocate");

  klee::ref<klee::Expr> index_range = call.args.at("index_range").expr;
  klee::ref<klee::Expr> chain_out   = call.args.at("chain_out").out;
  klee::ref<klee::Expr> key_size    = solver_toolbox.exprBuilder->Constant(32, klee::Expr::Int32);

  const std::optional<expiration_data_t> expiration_data = ctx.get_expiration_data();
  assert(expiration_data.has_value() && "Expiration data not found");

  const dchain_table_allocation_data_t data = {
      .obj             = expr_addr_to_obj_addr(chain_out),
      .key_size        = key_size,
      .capacity        = index_range,
      .expiration_time = expiration_data->expiration_time,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneDchainTableAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DataplaneDchainTableAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DataplaneDchainTableAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *dchain_allocate = dynamic_cast<const Call *>(node);
  const call_t &call          = dchain_allocate->get_call();

  if (call.function_name != "dchain_allocate") {
    return {};
  }

  const dchain_table_allocation_data_t data = get_dchain_table_data(ctx, dchain_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_DchainTable)) {
    return {};
  }

  return std::make_unique<DataplaneDchainTableAllocate>(type, node, data.obj, data.key_size, data.capacity, data.expiration_time);
}

} // namespace Controller
} // namespace LibSynapse