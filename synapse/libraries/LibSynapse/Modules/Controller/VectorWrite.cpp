#include <LibSynapse/Modules/Controller/VectorWrite.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::build_expr_mods;
using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> VectorWriteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> value            = call.args.at("value").in;
  const addr_t vector_addr               = expr_addr_to_obj_addr(vector_addr_expr);

  klee::ref<klee::Expr> original_value  = call_node->get_call().extra_vars.at("borrowed_cell").second;
  const std::vector<expr_mod_t> changes = build_expr_mods(original_value, value);

  // Check the Ignore module.
  if (changes.empty()) {
    return {};
  }

  if (!ctx.can_impl_ds(vector_addr, DSImpl::Controller_Vector)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> VectorWriteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr  = call.args.at("value").expr;
  klee::ref<klee::Expr> value            = call.args.at("value").in;

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);
  const addr_t value_addr  = expr_addr_to_obj_addr(value_addr_expr);

  if (!ep->get_ctx().can_impl_ds(vector_addr, DSImpl::Controller_Vector)) {
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

  Module *module  = new VectorWrite(node, vector_addr, index, value_addr, changes);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(vector_addr, DSImpl::Controller_Vector);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr  = call.args.at("value").expr;
  klee::ref<klee::Expr> value            = call.args.at("value").in;

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);
  const addr_t value_addr  = expr_addr_to_obj_addr(value_addr_expr);

  if (!ctx.check_ds_impl(vector_addr, DSImpl::Controller_Vector)) {
    return {};
  }

  const Call *vector_borrow = call_node->get_vector_borrow_from_return();
  assert(vector_borrow && "Vector return without borrow");

  klee::ref<klee::Expr> original_value  = vector_borrow->get_call().extra_vars.at("borrowed_cell").second;
  const std::vector<expr_mod_t> changes = build_expr_mods(original_value, value);

  if (changes.empty()) {
    return {};
  }

  return std::make_unique<VectorWrite>(node, vector_addr, index, value_addr, changes);
}

} // namespace Controller
} // namespace LibSynapse