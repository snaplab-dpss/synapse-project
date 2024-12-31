#include "else.h"

namespace tofino {

std::optional<spec_impl_t> ElseGenerator::speculate(const EP *ep,
                                                    const Node *node,
                                                    const Context &ctx) const {
  return std::nullopt;
}

std::vector<impl_t> ElseGenerator::process_node(const EP *ep,
                                                const Node *node) const {
  // Never explicitly generate this module from the BDD.
  return {};
}

} // namespace tofino
