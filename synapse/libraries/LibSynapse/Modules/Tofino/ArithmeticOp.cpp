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

compute_op_t build_op(const BDDNode *node) {
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return compute_op_t{.id = build_op_id(node), .kind = ComputeOpKind::ALU, .width = call.ret->getWidth()};
}

} // namespace

std::string ArithmeticOp::get_op_id() const { return build_op_id(node); }

std::optional<spec_impl_t> ArithmeticOpFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!matches(node)) {
    return {};
  }
  const call_t &call                   = dynamic_cast<const Call *>(node)->get_call();
  const compute_op_t op                = build_op(node);
  const std::unordered_set<DS_ID> deps = TofinoContext::get_dataflow_deps(ep, node, unrolled_op_value(call), speculations);
  return speculate_compute_step(ep, node, [&](ComputeStepBuilder &builder) { return builder.place(op, deps); }, speculations);
}

std::vector<impl_t> ArithmeticOpFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!matches(node)) {
    return {};
  }

  const call_t &call          = dynamic_cast<const Call *>(node)->get_call();
  klee::ref<klee::Expr> value = unrolled_op_value(call);
  klee::ref<klee::Expr> out   = call.ret;

  const compute_op_t op                = build_op(node);
  const std::unordered_set<DS_ID> deps = TofinoContext::get_dataflow_deps(ep, node, value);
  std::optional<compute_step_t> step   = implement_compute_step(ep, node, [&](ComputeStepBuilder &builder) { return builder.place(op, deps); });
  if (!step) {
    return {};
  }

  Module *module  = new ArithmeticOp(node, step->action_id, value, out);
  EPNode *ep_node = new EPNode(module);

  const EPLeaf leaf(ep_node, node->get_next());
  step->ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(step->ep)));
  return impls;
}

std::unique_ptr<Module> ArithmeticOpFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!matches(node)) {
    return {};
  }
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  const DS_ID action_id = ctx.get_target_ctx<TofinoContext>()->find_compute_action(build_op_id(node));
  return std::make_unique<ArithmeticOp>(node, action_id, unrolled_op_value(call), call.ret);
}

} // namespace Tofino
} // namespace LibSynapse
