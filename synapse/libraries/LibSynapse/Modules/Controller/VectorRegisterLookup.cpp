#include <LibSynapse/Modules/Controller/VectorRegisterLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> VectorRegisterLookupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return std::nullopt;
  }

  if (call_node->is_vector_borrow_value_ignored()) {
    return std::nullopt;
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  addr_t vector_addr                     = LibCore::expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> VectorRegisterLookupFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                              LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return impls;
  }

  if (call_node->is_vector_borrow_value_ignored()) {
    return impls;
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value            = call.extra_vars.at("borrowed_cell").second;

  addr_t vector_addr = LibCore::expr_addr_to_obj_addr(vector_addr_expr);

  if (!ep->get_ctx().check_ds_impl(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return impls;
  }

  Module *module  = new VectorRegisterLookup(node, vector_addr, index, value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> VectorRegisterLookupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (call_node->is_vector_borrow_value_ignored()) {
    return {};
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value            = call.extra_vars.at("borrowed_cell").second;

  addr_t vector_addr = LibCore::expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.check_ds_impl(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  return std::make_unique<VectorRegisterLookup>(node, vector_addr, index, value);
}

} // namespace Controller
} // namespace LibSynapse