#include <LibSynapse/Modules/Controller/DchainTableFreeIndex.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

namespace {

struct dchain_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> index;
};

dchain_table_data_t get_dchain_table_data(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "dchain_free_index" && "Not a dchain call");

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;

  dchain_table_data_t data = {
      .obj   = LibCore::expr_addr_to_obj_addr(dchain_addr_expr),
      .index = index,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DchainTableFreeIndexFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *dchain_free_index = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call            = dchain_free_index->get_call();

  if (call.function_name != "dchain_free_index") {
    return std::nullopt;
  }

  dchain_table_data_t data = get_dchain_table_data(dchain_free_index);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_DchainTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainTableFreeIndexFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                              LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *dchain_free_index = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call            = dchain_free_index->get_call();

  if (call.function_name != "dchain_free_index") {
    return impls;
  }

  dchain_table_data_t data = get_dchain_table_data(dchain_free_index);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_DchainTable)) {
    return impls;
  }

  Module *module  = new DchainTableFreeIndex(node, data.obj, data.index);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DchainTableFreeIndexFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *dchain_free_index = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call            = dchain_free_index->get_call();

  if (call.function_name != "dchain_free_index") {
    return {};
  }

  dchain_table_data_t data = get_dchain_table_data(dchain_free_index);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_DchainTable)) {
    return {};
  }

  return std::make_unique<DchainTableFreeIndex>(node, data.obj, data.index);
}

} // namespace Controller
} // namespace LibSynapse