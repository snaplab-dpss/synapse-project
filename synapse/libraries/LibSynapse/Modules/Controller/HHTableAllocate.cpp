#include <LibSynapse/Modules/Controller/HHTableAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using Tofino::DS;
using Tofino::DS_ID;
using Tofino::DSType;
using Tofino::HHTable;
using Tofino::Table;
using Tofino::TofinoContext;

namespace {

struct hh_table_allocation_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;
};

hh_table_allocation_data_t get_hh_table_allocation_data(const LibBDD::Call *map_allocate) {
  const LibBDD::call_t &call = map_allocate->get_call();
  assert(call.function_name == "map_allocate");

  klee::ref<klee::Expr> key_size   = call.args.at("key_size").expr;
  klee::ref<klee::Expr> capacity   = call.args.at("capacity").expr;
  klee::ref<klee::Expr> map_out    = call.args.at("map_out").out;
  klee::ref<klee::Expr> value_size = LibCore::solver_toolbox.exprBuilder->Constant(0, klee::Expr::Int32);

  addr_t obj = LibCore::expr_addr_to_obj_addr(map_out);

  return {obj, key_size, value_size, capacity};
}

} // namespace

std::optional<spec_impl_t> HHTableAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *map_allocate = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call       = map_allocate->get_call();

  if (call.function_name != "map_allocate") {
    return std::nullopt;
  }

  LibBDD::map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_map_op(map_allocate, map_objs)) {
    return std::nullopt;
  }

  if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> HHTableAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                         LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *map_allocate = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call       = map_allocate->get_call();

  if (call.function_name != "map_allocate") {
    return impls;
  }

  LibBDD::map_coalescing_objs_t map_objs;
  if (!ep->get_bdd()->get_map_coalescing_objs_from_map_op(map_allocate, map_objs)) {
    return impls;
  }

  if (!ep->get_ctx().check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable)) {
    return impls;
  }

  hh_table_allocation_data_t table_data = get_hh_table_allocation_data(map_allocate);

  Module *module  = new HHTableAllocate(node, table_data.obj, table_data.key_size, table_data.value_size, table_data.capacity);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> HHTableAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *map_allocate = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call       = map_allocate->get_call();

  if (call.function_name != "map_allocate") {
    return {};
  }

  LibBDD::map_coalescing_objs_t map_objs;
  if (!bdd->get_map_coalescing_objs_from_map_op(map_allocate, map_objs)) {
    return {};
  }

  if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable)) {
    return {};
  }

  hh_table_allocation_data_t table_data = get_hh_table_allocation_data(map_allocate);

  return std::make_unique<HHTableAllocate>(node, table_data.obj, table_data.key_size, table_data.value_size, table_data.capacity);
}

} // namespace Controller
} // namespace LibSynapse