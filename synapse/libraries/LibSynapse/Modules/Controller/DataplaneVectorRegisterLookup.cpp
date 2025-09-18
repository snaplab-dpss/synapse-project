#include <LibSynapse/Modules/Controller/DataplaneVectorRegisterLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> DataplaneVectorRegisterLookupFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (call_node->is_vector_borrow_value_ignored()) {
    return {};
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  const addr_t vector_addr               = expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneVectorRegisterLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (call_node->is_vector_borrow_value_ignored()) {
    return {};
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value            = call.extra_vars.at("borrowed_cell").second;

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

  if (!ep->get_ctx().check_ds_impl(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  Module *module  = new DataplaneVectorRegisterLookup(get_type().instance_id, node, vector_addr, index, value);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneVectorRegisterLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (call_node->is_vector_borrow_value_ignored()) {
    return {};
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value            = call.extra_vars.at("borrowed_cell").second;

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.check_ds_impl(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  return std::make_unique<DataplaneVectorRegisterLookup>(get_type().instance_id, node, vector_addr, index, value);
}

} // namespace Controller
} // namespace LibSynapse