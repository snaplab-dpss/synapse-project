#include <LibSynapse/Modules/Tofino/ArithmeticOp.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/Unroll.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;
using LibBDD::is_unrolled_op;
using LibBDD::unrolled_op_value;

namespace {

bool matches(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  if (!is_unrolled_op(call)) {
    return false;
  }
  // Operands must be readable in the data plane here (a reordering can bring us before the
  // register update that exposes a borrowed value).
  return !TofinoModuleFactory::reads_pending_write_borrow_value(node, unrolled_op_value(call));
}

std::string build_op_id(const BDDNode *node) {
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return call.function_name + "_" + std::to_string(node->get_id());
}

struct step_t {
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> out;
  compute_op_t op;
  std::vector<TofinoModuleFactory::compute_operand_t> operands; // Non-plain operands, computed first.
  std::vector<klee::ref<klee::Expr>> plain_operands;
};

step_t build_step(const BDDNode *node) {
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  step_t step;
  step.value = unrolled_op_value(call);
  step.out   = call.ret;
  step.op    = compute_op_t{
         .id      = build_op_id(node),
         .kind    = ComputeOpKind::ALU,
         .width   = call.ret->getWidth(),
         .fn      = call.function_name,
         .args    = {step.value},
         .out     = call.ret,
         .in_hash = false,
  };

  std::vector<std::pair<std::string, klee::ref<klee::Expr>>> kids;
  for (unsigned i = 0; i < step.value->getNumKids(); i++) {
    kids.emplace_back(i == 0 ? "_a" : "_b", step.value->getKid(i));
  }
  step.operands = TofinoModuleFactory::get_operands_to_compute(step.op.id, kids);
  for (const auto &[_, kid] : kids) {
    if (TofinoModuleFactory::is_plain_operand(kid)) {
      step.plain_operands.push_back(kid);
    }
  }
  return step;
}

// Places the operands to compute, then the op itself.
std::optional<DS_ID> place_step(TofinoModuleFactory::ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, step_t &step,
                                const speculations_t *speculations) {
  if (!TofinoModuleFactory::place_operand_ops(builder, ep, node, step.operands, speculations)) {
    return {};
  }
  return builder.place(step.op, TofinoModuleFactory::get_op_deps(ep, node, step.plain_operands, step.operands, speculations));
}

} // namespace

std::string ArithmeticOp::get_op_id() const { return build_op_id(node); }

std::optional<DS_ID> ArithmeticOpFactory::place(ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, const speculations_t *speculations) {
  if (!matches(node)) {
    return {};
  }
  step_t step = build_step(node);
  return place_step(builder, ep, node, step, speculations);
}

std::optional<spec_impl_t> ArithmeticOpFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!matches(node)) {
    return {};
  }
  return speculate_compute_run(ep, node, speculations);
}

std::vector<impl_t> ArithmeticOpFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!matches(node)) {
    return {};
  }

  step_t step = build_step(node);
  std::string why;
  std::optional<compute_step_t> impl_step = implement_compute_step(
      ep, node, [&](ComputeStepBuilder &builder) { return place_step(builder, ep, node, step, nullptr); }, &why);
  if (!impl_step) {
    return decline(why);
  }

  Module *module  = new ArithmeticOp(node, impl_step->action_id, step.value, step.out, step.operands);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  impl_step->ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(impl_step->ep)));
  return impls;
}

std::unique_ptr<Module> ArithmeticOpFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!matches(node)) {
    return {};
  }
  step_t step                     = build_step(node);
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  for (compute_operand_t &operand : step.operands) {
    operand.action_id = tofino_ctx->find_compute_action(operand.op_id);
  }
  return std::make_unique<ArithmeticOp>(node, tofino_ctx->find_compute_action(step.op.id), step.value, step.out, step.operands);
}

} // namespace Tofino
} // namespace LibSynapse
