#include <LibSynapse/Modules/Controller/DataplaneBloomFilterAllocate.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

namespace {

struct bf_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> height;
  klee::ref<klee::Expr> width;
  klee::ref<klee::Expr> key_size;
  time_ns_t cleanup_internal;
};

bf_allocation_data_t get_bf_allocatino_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "bf_allocate");

  klee::ref<klee::Expr> height                = call.args.at("height").expr;
  klee::ref<klee::Expr> width                 = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size              = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval_expr = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> bf_out                = call.args.at("bf_out").out;

  const time_ns_t cleanup_internal = solver_toolbox.value_from_expr(cleanup_interval_expr);

  const bf_allocation_data_t data = {
      .obj              = expr_addr_to_obj_addr(bf_out),
      .height           = height,
      .width            = width,
      .key_size         = key_size,
      .cleanup_internal = cleanup_internal,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneBloomFilterAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DataplaneBloomFilterAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DataplaneBloomFilterAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *bf_allocate = dynamic_cast<const Call *>(node);
  const call_t &call      = bf_allocate->get_call();

  if (call.function_name != "bf_allocate") {
    return {};
  }

  const bf_allocation_data_t data = get_bf_allocatino_data(ctx, bf_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_BloomFilter)) {
    return {};
  }

  return std::make_unique<DataplaneBloomFilterAllocate>(node, data.obj, data.height, data.width, data.key_size, data.cleanup_internal);
}

} // namespace Controller
} // namespace LibSynapse