#include <LibSynapse/Modules/Controller/DataplaneDchainTableAllocateNewIndex.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct dchain_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> success;
};

dchain_table_data_t get_dchain_table_data(const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "dchain_allocate_new_index" && "Not a dchain call");

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;
  const symbol_t not_out_of_space        = call_node->get_local_symbol("not_out_of_space");

  const dchain_table_data_t data = {
      .obj     = expr_addr_to_obj_addr(dchain_addr_expr),
      .index   = index_out,
      .success = not_out_of_space.expr,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneDchainTableAllocateNewIndexFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return {};
  }

  const dchain_table_data_t data = get_dchain_table_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_DchainTable)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneDchainTableAllocateNewIndexFactory::process_node(const EP *ep, const BDDNode *node,
                                                                              SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return {};
  }

  const dchain_table_data_t data = get_dchain_table_data(call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_DchainTable)) {
    return {};
  }

  Module *module  = new DataplaneDchainTableAllocateNewIndex(type, node, data.obj, data.index, data.success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneDchainTableAllocateNewIndexFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return {};
  }

  const dchain_table_data_t data = get_dchain_table_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_DchainTable)) {
    return {};
  }

  return std::make_unique<DataplaneDchainTableAllocateNewIndex>(type, node, data.obj, data.index, data.success);
}

} // namespace Controller
} // namespace LibSynapse