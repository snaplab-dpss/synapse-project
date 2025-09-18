#include <LibSynapse/Modules/Controller/DataplaneMeterAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

using Tofino::Table;

namespace {

struct tb_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> rate;
  klee::ref<klee::Expr> burst;
  klee::ref<klee::Expr> key_size;
};

tb_allocation_data_t get_tb_data(const Call *tb_allocate) {
  const call_t &call = tb_allocate->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate     = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst    = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out   = call.args.at("tb_out").out;

  const addr_t obj = expr_addr_to_obj_addr(tb_out);

  return {obj, capacity, rate, burst, key_size};
}

} // namespace

std::optional<spec_impl_t> DataplaneMeterAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *tb_allocate = dynamic_cast<const Call *>(node);
  const call_t &call      = tb_allocate->get_call();

  if (call.function_name != "tb_allocate") {
    return {};
  }

  const tb_allocation_data_t data = get_tb_data(tb_allocate);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_Meter)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneMeterAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *tb_allocate = dynamic_cast<const Call *>(node);
  const call_t &call      = tb_allocate->get_call();

  if (call.function_name != "tb_allocate") {
    return {};
  }

  const tb_allocation_data_t data = get_tb_data(tb_allocate);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_Meter)) {
    return {};
  }

  Module *module  = new DataplaneMeterAllocate(get_type().instance_id, node, data.obj, data.capacity, data.rate, data.burst, data.key_size);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneMeterAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *tb_allocate = dynamic_cast<const Call *>(node);
  const call_t &call      = tb_allocate->get_call();

  if (call.function_name != "tb_allocate") {
    return {};
  }

  const tb_allocation_data_t data = get_tb_data(tb_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_Meter)) {
    return {};
  }

  return std::make_unique<DataplaneMeterAllocate>(get_type().instance_id, node, data.obj, data.capacity, data.rate, data.burst, data.key_size);
}

} // namespace Controller
} // namespace LibSynapse