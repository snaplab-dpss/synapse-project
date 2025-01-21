#include <LibSynapse/Modules/Tofino/Then.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

std::optional<spec_impl_t> ThenFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const { return std::nullopt; }

std::vector<impl_t> ThenFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  // Never explicitly generate this module from the LibBDD::BDD.
  return {};
}

} // namespace Tofino
} // namespace LibSynapse