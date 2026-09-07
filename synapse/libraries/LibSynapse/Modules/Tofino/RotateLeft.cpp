#include <LibSynapse/Modules/Tofino/RotateLeft.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
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

DS_ID build_table_id(const BDDNode *node) { return "rotate_left_" + std::to_string(node->get_id()); }

} // namespace

std::optional<spec_impl_t> RotateLeftFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  const std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }
  Table *table = new Table(build_table_id(node), 1, {}, {rotation->out->getWidth()});
  return speculate_compute_step(ep, node, table, rotation->x, speculations);
}

std::vector<impl_t> RotateLeftFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  const std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }

  const DS_ID table_id = build_table_id(node);
  Table *table         = new Table(table_id, 1, {}, {rotation->out->getWidth()});

  // A stateless step only waits for the producer of its operand, so independent steps can
  // share a stage. When no stage can take it (the pipeline is used up), decline: the search
  // then recirculates or hands the rest to the controller.
  const std::unordered_set<DS_ID> deps = TofinoContext::get_dataflow_deps(ep, node, rotation->x);
  if (!ep->get_ctx().get_target_ctx<TofinoContext>()->can_place(ep, node, table, deps)) {
    delete table;
    return {};
  }

  Module *module             = new RotateLeft(node, table_id, rotation->x, rotation->amount, rotation->out);
  EPNode *ep_node            = new EPNode(module);
  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  TofinoContext *tofino_ctx = new_ep->get_mutable_ctx().get_mutable_target_ctx<TofinoContext>();
  tofino_ctx->place(new_ep.get(), node, node->get_id(), table, deps);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> RotateLeftFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  const std::optional<rotation_t> rotation = get_rotation(node);
  if (!rotation) {
    return {};
  }
  return std::make_unique<RotateLeft>(node, build_table_id(node), rotation->x, rotation->amount, rotation->out);
}

} // namespace Tofino
} // namespace LibSynapse
