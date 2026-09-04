#include <LibSynapse/Modules/Tofino/ComputeTableCountTrailingZeros.h>
#include <LibSynapse/Modules/Tofino/Helpers/ComputeTableEntries.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

static const char *FN = "count_trailing_zeros";

std::optional<spec_impl_t> CountTrailingZerosFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!compute_table_matches(node, FN)) {
    return {};
  }
  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> CountTrailingZerosFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!compute_table_matches(node, FN)) {
    return {};
  }

  const call_t &call        = dynamic_cast<const Call *>(node)->get_call();
  klee::ref<klee::Expr> in  = call.args.at("x").expr;
  klee::ref<klee::Expr> out = call.ret;

  auto *module               = new CountTrailingZeros(node, std::string(FN) + "_" + std::to_string(node->get_id()), in, out);
  std::unique_ptr<EP> new_ep = build_compute_table_ep(ep, node, module, in, TableMatch::Ternary, count_trailing_zeros_entries(in->getWidth()));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> CountTrailingZerosFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!compute_table_matches(node, FN)) {
    return {};
  }
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return std::make_unique<CountTrailingZeros>(node, std::string(FN) + "_" + std::to_string(node->get_id()), call.args.at("x").expr, call.ret);
}

} // namespace Tofino
} // namespace LibSynapse
