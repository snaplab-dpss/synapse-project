#include <LibSynapse/Modules/Controller/DataplaneFCFSCachedTableWrite.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> DataplaneFCFSCachedTableWriteFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DataplaneFCFSCachedTableWriteFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DataplaneFCFSCachedTableWriteFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  // We don't need this for now.
  return {};
}

} // namespace Controller
} // namespace LibSynapse