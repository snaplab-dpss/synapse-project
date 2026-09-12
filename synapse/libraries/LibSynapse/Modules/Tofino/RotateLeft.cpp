#include <LibSynapse/Modules/Tofino/RotateLeft.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;
using LibCore::is_constant;

namespace {

struct rotation_t {
  klee::ref<klee::Expr> x;
  u32 amount;
  klee::ref<klee::Expr> out;
  compute_op_t op;
  std::vector<TofinoModuleFactory::compute_operand_t> operands; // x, when it isn't a plain value.
};

std::string build_op_id(const BDDNode *node) { return "rotate_left_" + std::to_string(node->get_id()); }

// The rotation `node` computes, if it is one the data plane can do: a constant amount, and
// an operand readable here (a reordering can bring us before the register update that
// exposes a borrowed value).
std::optional<rotation_t> get_rotation(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  if (call.function_name != "rotate_left") {
    return {};
  }

  klee::ref<klee::Expr> x = call.args.at("x").expr;
  klee::ref<klee::Expr> n = call.args.at("n").expr;
  if (!is_constant(n) || TofinoModuleFactory::reads_pending_write_borrow_value(node, x)) {
    return {};
  }

  const bits_t width = call.ret->getWidth();
  const u32 amount   = solver_toolbox.value_from_expr(n) % width;

  rotation_t rotation{x, amount, call.ret, {}, {}};
  rotation.op = compute_op_t{
      .id      = build_op_id(node),
      .kind    = (amount % 8 != 0) ? ComputeOpKind::Hash : ComputeOpKind::ALU,
      .width   = width,
      .fn      = "rotate_left",
      .args    = {x, solver_toolbox.exprBuilder->Constant(amount, 32)},
      .out     = call.ret,
      .in_hash = amount != 0,
  };
  rotation.operands = TofinoModuleFactory::get_operands_to_compute(rotation.op.id, {{"_x", x}});
  return rotation;
}

std::optional<DS_ID> place_rotation(TofinoModuleFactory::ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, rotation_t &rotation,
                                    const speculations_t *speculations) {
  if (!TofinoModuleFactory::place_operand_ops(builder, ep, node, rotation.operands, speculations)) {
    return {};
  }
  const std::vector<klee::ref<klee::Expr>> plain =
      rotation.operands.empty() ? std::vector<klee::ref<klee::Expr>>{rotation.x} : std::vector<klee::ref<klee::Expr>>{};
  return builder.place(rotation.op, TofinoModuleFactory::get_op_deps(ep, node, plain, rotation.operands, speculations));
}

} // namespace

std::string RotateLeft::get_op_id() const { return build_op_id(node); }

std::optional<DS_ID> RotateLeftFactory::place(ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, const speculations_t *speculations) {
  std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }
  return place_rotation(builder, ep, node, *rotation, speculations);
}

std::optional<spec_impl_t> RotateLeftFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!get_rotation(node)) {
    return {};
  }
  return speculate_compute_run(ep, node, speculations);
}

std::vector<impl_t> RotateLeftFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }

  std::string why;
  std::optional<compute_step_t> step = implement_compute_step(
      ep, node, [&](ComputeStepBuilder &builder) { return place_rotation(builder, ep, node, *rotation, nullptr); }, &why);
  if (!step) {
    return decline(why);
  }

  Module *module  = new RotateLeft(node, step->action_id, rotation->x, rotation->amount, rotation->out, rotation->operands);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  step->ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(step->ep)));
  return impls;
}

std::unique_ptr<Module> RotateLeftFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  for (compute_operand_t &operand : rotation->operands) {
    operand.action_id = tofino_ctx->find_compute_action(operand.op_id);
  }
  return std::make_unique<RotateLeft>(node, tofino_ctx->find_compute_action(rotation->op.id), rotation->x, rotation->amount, rotation->out,
                                      rotation->operands);
}

} // namespace Tofino
} // namespace LibSynapse
