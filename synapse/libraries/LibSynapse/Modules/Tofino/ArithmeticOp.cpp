#include <LibSynapse/Modules/Tofino/ArithmeticOp.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibBDD/Unroll.h>

#include <algorithm>

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

// The clock shifted right by 16: the 32 bits the data plane keeps of it (the synthesizer's
// time_shift), copied out of the intrinsic's metadata field.
bool copies_clock(const BDD *bdd, klee::ref<klee::Expr> value) {
  std::string symbol;
  return value->getKind() == klee::Expr::LShr && LibCore::is_constant(value->getKid(1)) &&
         LibCore::solver_toolbox.value_from_expr(value->getKid(1)) == 16 && LibCore::is_readLSB(value->getKid(0), symbol) &&
         symbol == bdd->get_time().name;
}

step_t build_step(const BDD *bdd, const BDDNode *node) {
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
  const bool chain = TofinoModuleFactory::is_hash_chain_node(bdd, node);
  if (copies_clock(bdd, step.value)) {
    // The clock, read into the chain by the hash unit, as the ground truth's time_read does:
    // 32 bits of hash output, no operand to load.
    step.op.kind    = ComputeOpKind::Hash;
    step.op.width   = 32;
    step.op.in_hash = true;
  }

  std::vector<std::pair<std::string, klee::ref<klee::Expr>>> kids;
  for (unsigned i = 0; i < step.value->getNumKids(); i++) {
    kids.emplace_back(i == 0 ? "_a" : "_b", step.value->getKid(i));
  }
  // A chain xor reading an outside value is computed by the hash unit, operand and all; any
  // other chain op has its outside operands loaded into slots by the hash unit first (see
  // is_hash_chain_node). The clock's copy is that load already.
  bool outside = false;
  for (const auto &[_, kid] : kids) {
    outside |= chain && !LibCore::is_constant(kid) && TofinoModuleFactory::reads_outside(bdd, kid);
  }
  // After a chain, an xor is computed by the hash unit too (is_hash_chain_post); what it reads
  // -- slots, metadata -- the hash unit reads where it is.
  const bool post_xor = TofinoModuleFactory::is_hash_chain_post(bdd, node) && step.value->getKind() == klee::Expr::Xor;
  const bool hash_xor = ((chain && outside) || post_xor) && step.value->getKind() == klee::Expr::Xor && !step.op.in_hash;
  if (hash_xor) {
    step.op.kind    = ComputeOpKind::Hash;
    step.op.in_hash = true;
  }
  const TofinoModuleFactory::OutsideOperands mode = step.op.in_hash ? TofinoModuleFactory::OutsideOperands::Inline
                                                    : chain         ? TofinoModuleFactory::OutsideOperands::Load
                                                                    : TofinoModuleFactory::OutsideOperands::Keep;
  step.operands                                   = TofinoModuleFactory::get_operands_to_compute(step.op.id, kids, bdd, mode);
  for (const auto &[_, kid] : kids) {
    const bool computed = std::any_of(step.operands.begin(), step.operands.end(), [&](const auto &operand) { return operand.expr == kid; });
    if (!computed && TofinoModuleFactory::is_plain_operand(kid)) {
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
  step_t step = build_step(ep->get_bdd(), node);
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
  if (const std::optional<std::string> due = TofinoModuleFactory::loop_cut_due(ep, node)) {
    return decline(*due);
  }

  step_t step = build_step(ep->get_bdd(), node);
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
  step_t step                     = build_step(bdd, node);
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  for (compute_operand_t &operand : step.operands) {
    operand.action_id = tofino_ctx->find_compute_action(operand.op_id);
  }
  return std::make_unique<ArithmeticOp>(node, tofino_ctx->find_compute_action(step.op.id), step.value, step.out, step.operands);
}

} // namespace Tofino
} // namespace LibSynapse
