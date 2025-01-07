#include "ignore.h"

namespace synapse {
namespace x86 {
namespace {
bool should_ignore(const EP *ep, const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name == "vector_return") {
    return is_vector_return_without_modifications(ep, call_node);
  }

  return false;
}
} // namespace

std::optional<spec_impl_t> IgnoreFactory::speculate(const EP *ep, const Node *node,
                                                    const Context &ctx) const {
  return std::nullopt;
}

std::vector<impl_t> IgnoreFactory::process_node(const EP *ep, const Node *node,
                                                SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!should_ignore(ep, node)) {
    return impls;
  }

  Module *module = new Ignore(node);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace x86
} // namespace synapse