#include "checksum_update.h"

namespace synapse {
namespace x86 {
namespace {
bool bdd_node_match_pattern(const Node *node) {
  if (node->get_type() != NodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "nf_set_rte_ipv4_udptcp_checksum") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> ChecksumUpdateFactory::speculate(const EP *ep, const Node *node, const Context &ctx) const {
  if (bdd_node_match_pattern(node))
    return spec_impl_t(decide(ep, node), ctx);
  return std::nullopt;
}

std::vector<impl_t> ChecksumUpdateFactory::process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (!bdd_node_match_pattern(node)) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  klee::ref<klee::Expr> ip_hdr_addr_expr = call.args.at("ip_header").expr;
  klee::ref<klee::Expr> l4_hdr_addr_expr = call.args.at("l4_header").expr;
  klee::ref<klee::Expr> p = call.args.at("packet").expr;

  symbol_t checksum = call_node->get_local_symbol("checksum");
  addr_t ip_hdr_addr = expr_addr_to_obj_addr(ip_hdr_addr_expr);
  addr_t l4_hdr_addr = expr_addr_to_obj_addr(l4_hdr_addr_expr);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Module *module = new ChecksumUpdate(node, ip_hdr_addr, l4_hdr_addr, checksum);
  EPNode *ep_node = new EPNode(module);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace x86
} // namespace synapse