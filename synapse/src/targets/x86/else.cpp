#include "else.h"

namespace synapse {
namespace x86 {

std::optional<spec_impl_t> ElseFactory::speculate(const EP *ep, const Node *node,
                                                  const Context &ctx) const {
  return std::nullopt;
}

std::vector<impl_t> ElseFactory::process_node(const EP *ep, const Node *node) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

} // namespace x86
} // namespace synapse