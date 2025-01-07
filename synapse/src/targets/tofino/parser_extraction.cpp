#include "parser_extraction.h"

namespace synapse {
namespace tofino {

std::optional<spec_impl_t> ParserExtractionFactory::speculate(const EP *ep, const Node *node,
                                                              const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> ParserExtractionFactory::process_node(const EP *ep, const Node *node,
                                                          SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "packet_borrow_next_chunk") {
    return impls;
  }

  klee::ref<klee::Expr> hdr_addr_expr = call.args.at("chunk").out;
  klee::ref<klee::Expr> hdr = call.extra_vars.at("the_chunk").second;
  klee::ref<klee::Expr> length_expr = call.args.at("length").expr;

  // Relevant for IPv4 options, but left for future work.
  SYNAPSE_ASSERT(!call_node->is_hdr_parse_with_var_len(), "Not implemented");
  bytes_t length = solver_toolbox.value_from_expr(length_expr);

  addr_t hdr_addr = expr_addr_to_obj_addr(hdr_addr_expr);

  Module *module = new ParserExtraction(node, hdr_addr, hdr, length);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->parser_transition(ep, node, hdr);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino
} // namespace synapse