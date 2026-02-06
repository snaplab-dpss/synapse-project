#include <LibTessera/Modules/Controller/DataplaneFCFSCachedSetAllocate.h>
#include <LibTessera/ExecutionPlan.h>

namespace LibTessera {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

using Tofino::DS_ID;
using Tofino::FCFSCachedSet;

namespace {

struct fcfs_cs_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;
};

fcfs_cs_allocation_data_t get_fcfs_cs_allocation_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "map_allocate" && "Not a map_allocate call");

  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size   = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out    = call.args.at("map_out").out;
  klee::ref<klee::Expr> value_size = solver_toolbox.exprBuilder->Constant(0, klee::Expr::Int32);

  const addr_t obj = expr_addr_to_obj_addr(map_out);

  return {obj, key_size, value_size, capacity};
}

} // namespace

std::optional<spec_impl_t> DataplaneFCFSCachedSetAllocateFactory::speculate(const EP *ep, const BDDNode *node,
                                                                            const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  const addr_t map_addr               = expr_addr_to_obj_addr(map_addr_expr);

  if (!speculations.ctx.check_ds_impl(map_addr, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DataplaneFCFSCachedSetAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  const fcfs_cs_allocation_data_t data = get_fcfs_cs_allocation_data(ep->get_ctx(), call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  Module *module  = new DataplaneFCFSCachedSetAllocate(node, data.obj, data.key_size, data.value_size, data.capacity);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneFCFSCachedSetAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  const fcfs_cs_allocation_data_t data = get_fcfs_cs_allocation_data(ctx, call_node);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_FCFSCachedSet)) {
    return {};
  }

  return std::make_unique<DataplaneFCFSCachedSetAllocate>(node, data.obj, data.key_size, data.value_size, data.capacity);
}

} // namespace Controller
} // namespace LibTessera