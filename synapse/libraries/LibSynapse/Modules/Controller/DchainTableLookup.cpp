#include <LibSynapse/Modules/Controller/DchainTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

namespace {

struct dchain_table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  std::optional<LibCore::symbol_t> hit;
};

dchain_table_data_t get_dchain_table_data(const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert((call.function_name == "dchain_is_index_allocated" || call.function_name == "dchain_rejuvenate_index") && "Not a dchain call");

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;

  dchain_table_data_t data = {
      .obj   = LibCore::expr_addr_to_obj_addr(dchain_addr_expr),
      .key   = index,
      .value = nullptr,
      .hit   = std::nullopt,
  };

  if (call.function_name == "dchain_is_index_allocated") {
    data.hit = call_node->get_local_symbol("is_index_allocated");
  }

  return data;
}

} // namespace

std::optional<spec_impl_t> DchainTableLookupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated" && call.function_name != "dchain_rejuvenate_index") {
    return std::nullopt;
  }

  dchain_table_data_t data = get_dchain_table_data(call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_DchainTable)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DchainTableLookupFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                           LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated" && call.function_name != "dchain_rejuvenate_index") {
    return impls;
  }

  dchain_table_data_t data = get_dchain_table_data(call_node);

  if (!ep->get_ctx().check_ds_impl(data.obj, DSImpl::Tofino_DchainTable)) {
    return impls;
  }

  Module *module  = new DchainTableLookup(node, data.obj, data.key, data.value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DchainTableLookupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated" && call.function_name != "dchain_rejuvenate_index") {
    return {};
  }

  dchain_table_data_t data = get_dchain_table_data(call_node);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_DchainTable)) {
    return {};
  }

  return std::make_unique<DchainTableLookup>(node, data.obj, data.key, data.value);
}

} // namespace Controller
} // namespace LibSynapse