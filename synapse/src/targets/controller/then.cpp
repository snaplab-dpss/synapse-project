#include "then.h"

namespace synapse {
namespace ctrl {

std::optional<spec_impl_t> ThenFactory::speculate(const EP *ep, const Node *node, const Context &ctx) const {
  return std::nullopt;
}

std::vector<impl_t> ThenFactory::process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

} // namespace ctrl
} // namespace synapse