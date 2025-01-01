#include "cms_increment.h"

namespace tofino_cpu {

std::optional<spec_impl_t> CMSIncrementFactory::speculate(const EP *ep, const Node *node,
                                                          const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "cms_increment") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.can_impl_ds(cms_addr, DSImpl::TofinoCPU_CountMinSketch)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(cms_addr, DSImpl::TofinoCPU_CountMinSketch);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> CMSIncrementFactory::process_node(const EP *ep,
                                                      const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "cms_increment") {
    return impls;
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;

  addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

  if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::TofinoCPU_CountMinSketch)) {
    return impls;
  }

  Module *module = new CMSIncrement(node, cms_addr, key);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  new_ep->get_mutable_ctx().save_ds_impl(cms_addr, DSImpl::TofinoCPU_CountMinSketch);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino_cpu
