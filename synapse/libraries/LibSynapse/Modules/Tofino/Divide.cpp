#include <LibSynapse/Modules/Tofino/Divide.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

namespace {

struct divide_data_t {
  klee::ref<klee::Expr> denominator;
  u64 numerator;
  klee::ref<klee::Expr> quotient;
  DS_ID reg_id;
};

divide_data_t get_divide_data(const Call *node) {
  const call_t &call = node->get_call();

  klee::ref<klee::Expr> denominator     = call.args.at("denominator").expr;
  klee::ref<klee::Expr> numerator_expr  = call.args.at("numerator").expr;
  klee::ref<klee::Expr> quotient        = call.ret;

  const klee::ConstantExpr *numerator_const = dynamic_cast<klee::ConstantExpr *>(numerator_expr.get());
  assert(numerator_const && "Divide numerator must be a compile-time constant");

  const divide_data_t data = {
      .denominator = denominator,
      .numerator   = numerator_const->getZExtValue(),
      .quotient    = quotient,
      .reg_id      = "divide_" + std::to_string(node->get_id()),
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> DivideFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);

  if (call_node->get_call().function_name != "divide") {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DivideFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);

  if (call_node->get_call().function_name != "divide") {
    return {};
  }

  const divide_data_t data = get_divide_data(call_node);

  const tna_properties_t &properties = ep->get_ctx().get_target_ctx<TofinoContext>()->get_tna().tna_config.properties;
  Register *reg = new Register(properties, data.reg_id, 1, 8, data.quotient->getWidth(), {RegisterActionType::Read});

  Module *module  = new Divide(node, data.reg_id, data.denominator, data.numerator, data.quotient);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, node->get_id(), reg);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DivideFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);

  if (call_node->get_call().function_name != "divide") {
    return {};
  }

  const divide_data_t data = get_divide_data(call_node);

  return std::make_unique<Divide>(node, data.reg_id, data.denominator, data.numerator, data.quotient);
}

} // namespace Tofino
} // namespace LibSynapse
