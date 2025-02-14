#include <LibSynapse/Modules/Tofino/Else.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

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
  return std::make_unique<Else>(node);
}

} // namespace Tofino
} // namespace LibSynapse