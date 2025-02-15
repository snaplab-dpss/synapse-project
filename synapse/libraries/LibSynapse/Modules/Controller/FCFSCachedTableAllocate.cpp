#include <LibSynapse/Modules/Controller/FCFSCachedTableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS_ID;
using Tofino::FCFSCachedTable;

namespace {

struct fcfs_cached_table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;
};

fcfs_cached_table_allocation_data_t get_fcfs_cached_table_allocation_data(const Context &ctx, const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "map_allocate" && "Not a map_allocate call");

  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> key_size   = call.args.at("key_size").expr;
  klee::ref<klee::Expr> map_out    = call.args.at("map_out").out;
  klee::ref<klee::Expr> value_size = LibCore::solver_toolbox.exprBuilder->Constant(0, klee::Expr::Int32);

  addr_t obj = LibCore::expr_addr_to_obj_addr(map_out);

  return {obj, key_size, value_size, capacity};
}

} // namespace

std::optional<spec_impl_t> FCFSCachedTableAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_allocate") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
  addr_t map_addr                     = LibCore::expr_addr_to_obj_addr(map_addr_expr);

  if (!ctx.check_ds_impl(map_addr, DSImpl::Tofino_FCFSCachedTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> FCFSCachedTableAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                 LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_allocate") {
    return impls;
  }

  fcfs_cached_table_allocation_data_t data = get_fcfs_cached_table_allocation_data(ep->get_ctx(), call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_FCFSCachedTable)) {
    return impls;
  }

  Module *module  = new FCFSCachedTableAllocate(node, data.obj, data.key_size, data.value_size, data.capacity);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> FCFSCachedTableAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  fcfs_cached_table_allocation_data_t data = get_fcfs_cached_table_allocation_data(ctx, call_node);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_FCFSCachedTable)) {
    return {};
  }

  return std::make_unique<FCFSCachedTableAllocate>(node, data.obj, data.key_size, data.value_size, data.capacity);
}

} // namespace Controller
} // namespace LibSynapse