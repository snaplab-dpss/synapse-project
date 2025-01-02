#include "then.h"

namespace synapse {
namespace tofino_cpu {

std::optional<spec_impl_t> ThenFactory::speculate(const EP *ep, const Node *node,
                                                  const Context &ctx) const {
  return std::nullopt;
}

std::vector<impl_t> ThenFactory::process_node(const EP *ep, const Node *node) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

} // namespace tofino_cpu
} // namespace synapse