#include <LibSynapse/Modules/Controller/Else.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> ElseFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const { return std::nullopt; }

std::vector<impl_t> ElseFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  // Never explicitly generate this module from the LibBDD::BDD.
  return {};
}

} // namespace Controller
} // namespace LibSynapse