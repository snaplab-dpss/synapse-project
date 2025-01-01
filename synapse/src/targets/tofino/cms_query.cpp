#include "cms_query.h"

namespace tofino {

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
  klee::ref<klee::Expr> key = call.args.at("key").in;

  addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.can_impl_ds(cms_addr, DSImpl::Tofino_CountMinSketch)) {
    return std::nullopt;
  }

  const cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_addr);
  std::vector<klee::ref<klee::Expr>> keys = Table::build_keys(key);

  if (!can_build_or_reuse_cms(ep, node, cms_addr, keys, cfg.width,
                              cfg.height)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch);

  return spec_impl_t(decide(ep, node), new_ctx);
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

  if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::Tofino_CountMinSketch)) {
    return impls;
  }

  const cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_addr);
  std::vector<klee::ref<klee::Expr>> keys = Table::build_keys(key);

  CountMinSketch *cms =
      build_or_reuse_cms(ep, node, cms_addr, keys, cfg.width, cfg.height);

  if (!cms) {
    return impls;
  }

  Module *module =
      new CMSQuery(node, cms->id, cms_addr, key, min_estimate.expr);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, cms_addr, cms);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino
