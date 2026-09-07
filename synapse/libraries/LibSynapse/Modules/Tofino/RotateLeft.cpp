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
};

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
  return rotation_t{x, amount, call.ret};
}

std::string build_op_id(const BDDNode *node) { return "rotate_left_" + std::to_string(node->get_id()); }

compute_op_t build_op(const BDDNode *node, const rotation_t &rotation) {
  const ComputeOpKind kind = (rotation.amount % 8 != 0) ? ComputeOpKind::Hash : ComputeOpKind::ALU;
  return compute_op_t{.id = build_op_id(node), .kind = kind, .width = rotation.out->getWidth()};
}

} // namespace

std::string RotateLeft::get_op_id() const { return build_op_id(node); }

std::optional<spec_impl_t> RotateLeftFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  const std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }
  const compute_op_t op                = build_op(node, *rotation);
  const std::unordered_set<DS_ID> deps = TofinoContext::get_dataflow_deps(ep, node, rotation->x, speculations);
  return speculate_compute_step(ep, node, [&](ComputeStepBuilder &builder) { return builder.place(op, deps); }, speculations);
}

std::vector<impl_t> RotateLeftFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }

  const compute_op_t op                = build_op(node, *rotation);
  const std::unordered_set<DS_ID> deps = TofinoContext::get_dataflow_deps(ep, node, rotation->x);
  std::optional<compute_step_t> step   = implement_compute_step(ep, node, [&](ComputeStepBuilder &builder) { return builder.place(op, deps); });
  if (!step) {
    return {};
  }

  Module *module  = new RotateLeft(node, step->action_id, rotation->x, rotation->amount, rotation->out);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  step->ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(step->ep)));
  return impls;
}

std::unique_ptr<Module> RotateLeftFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  const std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }
  const DS_ID action_id = ctx.get_target_ctx<TofinoContext>()->find_compute_action(build_op_id(node));
  return std::make_unique<RotateLeft>(node, action_id, rotation->x, rotation->amount, rotation->out);
}

} // namespace Tofino
} // namespace LibSynapse
