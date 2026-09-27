#include <LibSynapse/Modules/Controller/DataplaneLPMAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct lpm_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> key_size;
};

lpm_allocation_data_t get_lpm_data(const Call *lpm_allocate) {
  const call_t &call = lpm_allocate->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> lpm_out  = call.args.at("lpm_out").out;

  const addr_t obj = expr_addr_to_obj_addr(lpm_out);

  return {obj, capacity, key_size};
}

} // namespace

std::optional<spec_impl_t> DataplaneLPMAllocateFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_allocate = dynamic_cast<const Call *>(node);
  const call_t &call       = lpm_allocate->get_call();

  if (call.function_name != "lpm_allocate") {
    return {};
  }

  const lpm_allocation_data_t data = get_lpm_data(lpm_allocate);

  if (!speculations.ctx.can_impl_ds(data.obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DataplaneLPMAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_allocate = dynamic_cast<const Call *>(node);
  const call_t &call       = lpm_allocate->get_call();

  if (call.function_name != "lpm_allocate") {
    return {};
  }

  const lpm_allocation_data_t data = get_lpm_data(lpm_allocate);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  Module *module  = new DataplaneLPMAllocate(node, data.obj, data.capacity, data.key_size);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneLPMAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_allocate = dynamic_cast<const Call *>(node);
  const call_t &call       = lpm_allocate->get_call();

  if (call.function_name != "lpm_allocate") {
    return {};
  }

  const lpm_allocation_data_t data = get_lpm_data(lpm_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  return std::make_unique<DataplaneLPMAllocate>(node, data.obj, data.capacity, data.key_size);
}

} // namespace Controller
} // namespace LibSynapse
