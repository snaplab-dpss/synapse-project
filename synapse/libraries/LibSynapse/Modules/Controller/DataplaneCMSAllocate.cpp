#include <LibSynapse/Modules/Controller/DataplaneCMSAllocate.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

namespace {

struct cms_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> height;
  klee::ref<klee::Expr> width;
  klee::ref<klee::Expr> key_size;
  time_ns_t cleanup_internal;
};

cms_allocation_data_t get_cms_allocatino_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "cms_allocate");

  klee::ref<klee::Expr> height                = call.args.at("height").expr;
  klee::ref<klee::Expr> width                 = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size              = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval_expr = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> cms_out               = call.args.at("cms_out").out;

  const time_ns_t cleanup_internal = solver_toolbox.value_from_expr(cleanup_interval_expr);

  const cms_allocation_data_t data = {
      .obj              = expr_addr_to_obj_addr(cms_out),
      .height           = height,
      .width            = width,
      .key_size         = key_size,
      .cleanup_internal = cleanup_internal,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneCMSAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DataplaneCMSAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DataplaneCMSAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *cms_allocate = dynamic_cast<const Call *>(node);
  const call_t &call       = cms_allocate->get_call();

  if (call.function_name != "cms_allocate") {
    return {};
  }

  const cms_allocation_data_t data = get_cms_allocatino_data(ctx, cms_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_CountMinSketch)) {
    return {};
  }

  return std::make_unique<DataplaneCMSAllocate>(get_type().instance_id, node, data.obj, data.height, data.width, data.key_size,
                                                data.cleanup_internal);
}

} // namespace Controller
} // namespace LibSynapse