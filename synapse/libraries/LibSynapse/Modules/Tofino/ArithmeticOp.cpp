#include <LibSynapse/Modules/Tofino/ArithmeticOp.h>
#include <LibSynapse/Modules/Tofino/TofinoContext.h>
#include <LibSynapse/Modules/Tofino/DataStructures/Table.h>
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

DS_ID build_table_id(const BDDNode *node) {
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return call.function_name + "_" + std::to_string(node->get_id());
}

} // namespace

std::optional<spec_impl_t> ArithmeticOpFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!matches(node)) {
    return {};
  }
  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> ArithmeticOpFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!matches(node)) {
    return {};
  }

  const call_t &call          = dynamic_cast<const Call *>(node)->get_call();
  klee::ref<klee::Expr> value = unrolled_op_value(call);
  klee::ref<klee::Expr> out   = call.ret;
  const DS_ID table_id        = build_table_id(node);

  Table *table = new Table(table_id, 1, {}, {out->getWidth()});

  // A stateless step only waits for the producers of its operands, so independent steps can
  // share a stage. When no stage can take it (the pipeline is used up), decline: the search
  // then recirculates or hands the rest to the controller.
  const std::unordered_set<DS_ID> deps = TofinoContext::get_dataflow_deps(ep, node, value);
  if (!ep->get_ctx().get_target_ctx<TofinoContext>()->can_place(ep, node, table, deps)) {
    delete table;
    return {};
  }

  Module *module             = new ArithmeticOp(node, table_id, value, out);
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

std::unique_ptr<Module> ArithmeticOpFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!matches(node)) {
    return {};
  }
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return std::make_unique<ArithmeticOp>(node, build_table_id(node), unrolled_op_value(call), call.ret);
}

} // namespace Tofino
} // namespace LibSynapse
