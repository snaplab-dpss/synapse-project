#include <LibSynapse/Modules/Controller/DataplaneVectorRegisterUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::build_expr_mods;
using LibCore::expr_addr_to_obj_addr;
using LibCore::expr_mod_t;

std::optional<spec_impl_t> DataplaneVectorRegisterUpdateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  const addr_t vector_addr               = expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> DataplaneVectorRegisterUpdateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  klee::ref<klee::Expr> obj_expr        = call.args.at("vector").expr;
  klee::ref<klee::Expr> index           = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr = call.args.at("value").expr;
  klee::ref<klee::Expr> value           = call.args.at("value").in;

  const addr_t obj        = expr_addr_to_obj_addr(obj_expr);
  const addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

  if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  const Call *vector_borrow = call_node->get_vector_borrow_from_return();
  assert(vector_borrow && "Vector return without borrow");

  klee::ref<klee::Expr> original_value  = vector_borrow->get_call().extra_vars.at("borrowed_cell").second;
  const std::vector<expr_mod_t> changes = build_expr_mods(original_value, value);

  // Check the Ignore module.
  if (changes.empty()) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Module *module  = new DataplaneVectorRegisterUpdate(type, node, obj, index, value_addr, original_value, value, changes);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneVectorRegisterUpdateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  klee::ref<klee::Expr> obj_expr        = call.args.at("vector").expr;
  klee::ref<klee::Expr> index           = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr = call.args.at("value").expr;
  klee::ref<klee::Expr> value           = call.args.at("value").in;

  const addr_t obj        = expr_addr_to_obj_addr(obj_expr);
  const addr_t value_addr = expr_addr_to_obj_addr(value_addr_expr);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  const Call *vector_borrow = call_node->get_vector_borrow_from_return();
  assert(vector_borrow && "Vector return without borrow");

  klee::ref<klee::Expr> original_value  = vector_borrow->get_call().extra_vars.at("borrowed_cell").second;
  const std::vector<expr_mod_t> changes = build_expr_mods(original_value, value);

  if (changes.empty()) {
    return {};
  }

  return std::make_unique<DataplaneVectorRegisterUpdate>(type, node, obj, index, value_addr, original_value, value, changes);
}

} // namespace Controller
} // namespace LibSynapse