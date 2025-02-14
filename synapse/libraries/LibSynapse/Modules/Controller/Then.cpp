#include <LibSynapse/Modules/Controller/Then.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> ThenFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  // Never explicitly generate this module from the BDD.
  return std::nullopt;
}

std::vector<impl_t> ThenFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

std::unique_ptr<Module> ThenFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

} // namespace Controller
} // namespace LibSynapse