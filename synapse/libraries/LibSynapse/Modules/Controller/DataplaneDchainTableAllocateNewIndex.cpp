#include <LibSynapse/Modules/Controller/DataplaneDchainTableAllocateNewIndex.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

namespace {

struct dchain_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> success;
};

dchain_table_data_t get_dchain_table_data(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "dchain_allocate_new_index" && "Not a dchain call");

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index_out        = call.args.at("index_out").out;
  LibCore::symbol_t not_out_of_space     = call_node->get_local_symbol("not_out_of_space");

  dchain_table_data_t data = {
      .obj     = LibCore::expr_addr_to_obj_addr(dchain_addr_expr),
      .index   = index_out,
      .success = not_out_of_space.expr,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DataplaneDchainTableAllocateNewIndexFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return std::nullopt;
  }

  dchain_table_data_t data = get_dchain_table_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_DchainTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneDchainTableAllocateNewIndexFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                              LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return impls;
  }

  dchain_table_data_t data = get_dchain_table_data(call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_DchainTable)) {
    return impls;
  }

  Module *module  = new DataplaneDchainTableAllocateNewIndex(node, data.obj, data.index, data.success);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DataplaneDchainTableAllocateNewIndexFactory::create(const LibBDD::BDD *bdd, const Context &ctx,
                                                                            const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_allocate_new_index") {
    return {};
  }

  dchain_table_data_t data = get_dchain_table_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_DchainTable)) {
    return {};
  }

  return std::make_unique<DataplaneDchainTableAllocateNewIndex>(node, data.obj, data.index, data.success);
}

} // namespace Controller
} // namespace LibSynapse