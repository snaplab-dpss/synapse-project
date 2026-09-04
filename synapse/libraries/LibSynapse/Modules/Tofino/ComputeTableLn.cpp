#include <LibSynapse/Modules/Tofino/ComputeTableLn.h>
#include <LibSynapse/Modules/Tofino/Helpers/ComputeTableEntries.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

static const char *FN = "ln";

std::optional<spec_impl_t> LnFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!compute_table_matches(node, FN)) {
    return {};
  }
  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> LnFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!compute_table_matches(node, FN)) {
    return {};
  }

  const call_t &call        = dynamic_cast<const Call *>(node)->get_call();
  klee::ref<klee::Expr> in  = call.args.at("x").expr;
  klee::ref<klee::Expr> out = call.ret;

  // ln entries come from the values the profiler observed for this node.
  const std::vector<table_entry_t> entries = ln_entries(ep->get_ctx().get_profiler().get_ln_inputs(node));

  auto *module               = new Ln(node, std::string(FN) + "_" + std::to_string(node->get_id()), in, out);
  std::unique_ptr<EP> new_ep = build_compute_table_ep(ep, node, module, in, TableMatch::Exact, entries);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> LnFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!compute_table_matches(node, FN)) {
    return {};
  }
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return std::make_unique<Ln>(node, std::string(FN) + "_" + std::to_string(node->get_id()), call.args.at("x").expr, call.ret);
}

} // namespace Tofino
} // namespace LibSynapse
