#include <LibSynapse/Modules/Controller/DataplaneMeterAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::Table;

namespace {

struct tb_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> rate;
  klee::ref<klee::Expr> burst;
  klee::ref<klee::Expr> key_size;
};

tb_allocation_data_t get_tb_data(const LibBDD::Call *tb_allocate) {
  const LibBDD::call_t &call = tb_allocate->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate     = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst    = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out   = call.args.at("tb_out").out;

  addr_t obj = LibCore::expr_addr_to_obj_addr(tb_out);

  return {obj, capacity, rate, burst, key_size};
}

} // namespace

std::optional<spec_impl_t> DataplaneMeterAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *tb_allocate = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call      = tb_allocate->get_call();

  if (call.function_name != "tb_allocate") {
    return std::nullopt;
  }

  tb_allocation_data_t data = get_tb_data(tb_allocate);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_Meter)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneMeterAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *tb_allocate = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call      = tb_allocate->get_call();

  if (call.function_name != "tb_allocate") {
    return impls;
  }

  tb_allocation_data_t data = get_tb_data(tb_allocate);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_Meter)) {
    return impls;
  }

  Module *module  = new DataplaneMeterAllocate(node, data.obj, data.capacity, data.rate, data.burst, data.key_size);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DataplaneMeterAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *tb_allocate = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call      = tb_allocate->get_call();

  if (call.function_name != "tb_allocate") {
    return {};
  }

  tb_allocation_data_t data = get_tb_data(tb_allocate);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_Meter)) {
    return {};
  }

  return std::make_unique<DataplaneMeterAllocate>(node, data.obj, data.capacity, data.rate, data.burst, data.key_size);
}

} // namespace Controller
} // namespace LibSynapse