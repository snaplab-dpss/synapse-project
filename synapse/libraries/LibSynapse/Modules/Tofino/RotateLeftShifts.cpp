#include <LibSynapse/Modules/Tofino/RotateLeftShifts.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;
using LibCore::is_constant;

namespace {

std::string build_op_id(const BDDNode *node, const std::string &suffix) { return "rotate_left_" + std::to_string(node->get_id()) + suffix; }

struct rotation_t {
  klee::ref<klee::Expr> x;
  u32 amount;
  klee::ref<klee::Expr> out;
  std::vector<TofinoModuleFactory::compute_operand_t> operands; // x, when it isn't a plain value.
};

// The rotation `node` computes, if it is one worth doing with shifts: a constant amount that
// is not a whole number of bytes (those are plain moves, RotateLeft's job), and an operand
// readable here (a reordering can bring us before the register update that exposes a
// borrowed value).
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
  if (amount % 8 == 0) {
    return {};
  }

  rotation_t rotation{x, amount, call.ret, {}};
  rotation.operands = TofinoModuleFactory::get_operands_to_compute(build_op_id(node, ""), {{"_x", x}});
  return rotation;
}

struct placed_t {
  DS_ID shl;
  DS_ID shr;
  DS_ID out;
};

// Computes x if needed, then the two shifts (same deps), then the OR of both.
std::optional<placed_t> place_ops(TofinoModuleFactory::ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, rotation_t &rotation,
                                  const speculations_t *speculations) {
  if (!TofinoModuleFactory::place_operand_ops(builder, ep, node, rotation.operands, speculations)) {
    return {};
  }
  const std::vector<klee::ref<klee::Expr>> plain = rotation.operands.empty() ? std::vector<klee::ref<klee::Expr>>{rotation.x} : std::vector<klee::ref<klee::Expr>>{};
  const std::unordered_set<DS_ID> deps           = TofinoModuleFactory::get_op_deps(ep, node, plain, rotation.operands, speculations);
  const bits_t width                             = rotation.out->getWidth();

  const std::optional<DS_ID> shl = builder.place({.id = build_op_id(node, "_shl"), .kind = ComputeOpKind::ALU, .width = width}, deps);
  const std::optional<DS_ID> shr = builder.place({.id = build_op_id(node, "_shr"), .kind = ComputeOpKind::ALU, .width = width}, deps);
  if (!shl || !shr) {
    return {};
  }

  const std::optional<DS_ID> out = builder.place({.id = build_op_id(node, "_or"), .kind = ComputeOpKind::ALU, .width = width}, {*shl, *shr});
  if (!out) {
    return {};
  }

  return placed_t{*shl, *shr, *out};
}

} // namespace

std::optional<DS_ID> RotateLeftShiftsFactory::place(ComputeStepBuilder &builder, const EP *ep, const BDDNode *node, const speculations_t *speculations) {
  std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }
  const std::optional<placed_t> placed = place_ops(builder, ep, node, *rotation, speculations);
  if (!placed) {
    return {};
  }
  return placed->out;
}

std::string RotateLeftShifts::get_shl_op_id() const { return build_op_id(node, "_shl"); }
std::string RotateLeftShifts::get_shr_op_id() const { return build_op_id(node, "_shr"); }
std::string RotateLeftShifts::get_or_op_id() const { return build_op_id(node, "_or"); }

std::optional<spec_impl_t> RotateLeftShiftsFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  // Speculated by the compute run started from RotateLeft, which falls back to the shift form
  // when the hash unit can't take the rotation; a second run candidate would only tie with it.
  return {};
}

std::vector<impl_t> RotateLeftShiftsFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }

  std::optional<placed_t> placed;
  std::string why;
  std::optional<compute_step_t> step = implement_compute_step(
      ep, node,
      [&](ComputeStepBuilder &builder) -> std::optional<DS_ID> {
        placed = place_ops(builder, ep, node, *rotation, nullptr);
        if (!placed) {
          return {};
        }
        return placed->out;
      },
      &why);
  if (!step) {
    return decline(why);
  }

  Module *module =
      new RotateLeftShifts(node, placed->shl, placed->shr, placed->out, rotation->x, rotation->amount, rotation->out, rotation->operands);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  step->ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(step->ep)));
  return impls;
}

std::unique_ptr<Module> RotateLeftShiftsFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }
  const TofinoContext *tofino_ctx = ctx.get_target_ctx<TofinoContext>();
  for (compute_operand_t &operand : rotation->operands) {
    operand.action_id = tofino_ctx->find_compute_action(operand.op_id);
  }
  return std::make_unique<RotateLeftShifts>(node, tofino_ctx->find_compute_action(build_op_id(node, "_shl")),
                                            tofino_ctx->find_compute_action(build_op_id(node, "_shr")),
                                            tofino_ctx->find_compute_action(build_op_id(node, "_or")), rotation->x, rotation->amount, rotation->out,
                                            rotation->operands);
}

} // namespace Tofino
} // namespace LibSynapse
