#include "checksum_update.h"

namespace tofino_cpu {

std::optional<spec_impl_t>
ChecksumUpdateGenerator::speculate(const EP *ep, const Node *node,
                                   const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "nf_set_rte_ipv4_udptcp_checksum") {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t>
ChecksumUpdateGenerator::process_node(const EP *ep, const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "nf_set_rte_ipv4_udptcp_checksum") {
    return impls;
  }

  klee::ref<klee::Expr> ip_hdr_addr_expr = call.args.at("ip_header").expr;
  klee::ref<klee::Expr> l4_hdr_addr_expr = call.args.at("l4_header").expr;
  klee::ref<klee::Expr> p = call.args.at("packet").expr;

  symbols_t symbols = call_node->get_locally_generated_symbols();
  symbol_t checksum;
  bool found = get_symbol(symbols, "checksum", checksum);
  ASSERT(found, "Symbol checksum not found");

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

} // namespace tofino_cpu
