#include <LibTessera/Modules/Controller/Then.h>
#include <LibTessera/ExecutionPlan.h>

namespace LibTessera {
namespace Controller {

std::optional<spec_impl_t> ThenFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

std::vector<impl_t> ThenFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

std::unique_ptr<Module> ThenFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

} // namespace Controller
} // namespace LibTessera