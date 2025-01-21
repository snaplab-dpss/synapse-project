#include <LibSynapse/Modules/Controller/VectorRegisterUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> VectorRegisterUpdateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  addr_t vector_addr                     = LibCore::expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> VectorRegisterUpdateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                              LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return impls;
  }

  klee::ref<klee::Expr> obj_expr        = call.args.at("vector").expr;
  klee::ref<klee::Expr> index           = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr = call.args.at("value").expr;
  klee::ref<klee::Expr> value           = call.args.at("value").in;

  addr_t obj        = LibCore::expr_addr_to_obj_addr(obj_expr);
  addr_t value_addr = LibCore::expr_addr_to_obj_addr(value_addr_expr);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_VectorRegister)) {
    return impls;
  }

  const LibBDD::Call *vector_borrow = call_node->get_vector_borrow_from_return();
  assert(vector_borrow && "Vector return without borrow");

  klee::ref<klee::Expr> original_value     = vector_borrow->get_call().extra_vars.at("borrowed_cell").second;
  std::vector<LibCore::expr_mod_t> changes = LibCore::build_expr_mods(original_value, value);

  // Check the Ignore module.
  if (changes.empty()) {
    return impls;
  }

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *module  = new VectorRegisterUpdate(node, obj, index, value_addr, changes);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace Controller
} // namespace LibSynapse