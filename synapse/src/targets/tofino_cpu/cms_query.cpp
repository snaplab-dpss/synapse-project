#include "cms_query.h"

namespace tofino_cpu {

std::optional<spec_impl_t>
CMSQueryFactory::speculate(const EP *ep, const Node *node,
                           const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.check_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> CMSQueryFactory::process_node(const EP *ep,
                                                  const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return impls;
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;

  addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

  symbols_t symbols = call_node->get_locally_generated_symbols();
  symbol_t min_estimate;
  bool found = get_symbol(symbols, "min_estimate", min_estimate);
  ASSERT(found, "Symbol min_estimate not found");

  if (!ep->get_ctx().check_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch)) {
    return impls;
  }

  Module *module = new CMSQuery(node, cms_addr, key, min_estimate.expr);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino_cpu
