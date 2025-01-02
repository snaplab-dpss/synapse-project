#include "modify_header.h"

namespace synapse {
namespace x86 {
namespace {
bool bdd_node_match_pattern(const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "packet_return_chunk") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ModifyHeaderFactory::speculate(const EP *ep, const Node *node,
                                                          const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> ModifyHeaderFactory::process_node(const EP *ep,
                                                      const Node *node) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const Call *packet_return_chunk = dynamic_cast<const Call *>(node);
  const call_t &call = packet_return_chunk->get_call();

  if (call.function_name != "packet_return_chunk") {
    return impls;
  }

  const Call *packet_borrow_chunk = packet_borrow_from_return(ep, packet_return_chunk);
  SYNAPSE_ASSERT(packet_borrow_chunk,
                 "Failed to find packet_borrow_next_chunk from packet_return_chunk");

  klee::ref<klee::Expr> hdr = call.args.at("the_chunk").expr;
  addr_t hdr_addr = expr_addr_to_obj_addr(hdr);

  std::vector<mod_t> changes =
      build_hdr_modifications(packet_borrow_chunk, packet_return_chunk);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  if (changes.size() == 0) {
    new_ep->process_leaf(node->get_next());
    return impls;
  }

  Module *module = new ModifyHeader(node, hdr_addr, changes);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace x86
} // namespace synapse