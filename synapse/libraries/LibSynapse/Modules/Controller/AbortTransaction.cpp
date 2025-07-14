#include <LibSynapse/Modules/Controller/AbortTransaction.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> AbortTransactionFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

std::vector<impl_t> AbortTransactionFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

std::unique_ptr<Module> AbortTransactionFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

} // namespace Controller
} // namespace LibSynapse