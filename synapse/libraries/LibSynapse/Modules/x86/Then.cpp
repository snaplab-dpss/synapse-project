#include <LibSynapse/Modules/x86/Then.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace x86 {

std::optional<spec_impl_t> ThenFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const { return std::nullopt; }

std::vector<impl_t> ThenFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  // Never explicitly generate this module from the LibBDD::BDD.
  return {};
}

} // namespace x86
} // namespace LibSynapse