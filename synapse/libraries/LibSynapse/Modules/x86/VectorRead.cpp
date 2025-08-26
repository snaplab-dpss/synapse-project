#include <LibSynapse/Modules/x86/VectorRead.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> VectorReadFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  const addr_t vector_addr               = expr_addr_to_obj_addr(vector_addr_expr);

  if (!ctx.can_impl_ds(vector_addr, DSImpl::x86_Vector)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> VectorReadFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr  = call.args.at("val_out").out;
  klee::ref<klee::Expr> value            = call.extra_vars.at("borrowed_cell").second;

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);
  const addr_t value_addr  = expr_addr_to_obj_addr(value_addr_expr);

  if (!ep->get_ctx().can_impl_ds(vector_addr, DSImpl::x86_Vector)) {
    return {};
  }

  Module *module  = new VectorRead(type, node, vector_addr, index, value_addr, value);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(vector_addr, DSImpl::x86_Vector);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorReadFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> value_addr_expr  = call.args.at("val_out").out;
  klee::ref<klee::Expr> value            = call.extra_vars.at("borrowed_cell").second;

  const addr_t vector_addr = expr_addr_to_obj_addr(vector_addr_expr);
  const addr_t value_addr  = expr_addr_to_obj_addr(value_addr_expr);

  if (!ctx.check_ds_impl(vector_addr, DSImpl::x86_Vector)) {
    return {};
  }

  return std::make_unique<VectorRead>(type, node, vector_addr, index, value_addr, value);
}

} // namespace x86
} // namespace LibSynapse