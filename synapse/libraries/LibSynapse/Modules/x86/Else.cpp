#include <LibSynapse/Modules/x86/Else.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace x86 {

std::optional<spec_impl_t> ElseFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  // Never explicitly generate this module from the BDD.
  return std::nullopt;
}

std::vector<impl_t> ElseFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

std::unique_ptr<Module> ElseFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

} // namespace x86
} // namespace LibSynapse