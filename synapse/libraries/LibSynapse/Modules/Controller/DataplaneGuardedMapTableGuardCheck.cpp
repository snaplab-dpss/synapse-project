#include <LibSynapse/Modules/Controller/DataplaneGuardedMapTableGuardCheck.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> DataplaneGuardedMapTableGuardCheckFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  // We don't need this for now.
  return {};
}

std::vector<impl_t> DataplaneGuardedMapTableGuardCheckFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                                            LibCore::SymbolManager *symbol_manager) const {
  // We don't need this for now.
  return {};
}

std::unique_ptr<Module> DataplaneGuardedMapTableGuardCheckFactory::create(const LibBDD::BDD *bdd, const Context &ctx,
                                                                          const LibBDD::Node *node) const {
  // We don't need this for now.
  return {};
}

} // namespace Controller
} // namespace LibSynapse