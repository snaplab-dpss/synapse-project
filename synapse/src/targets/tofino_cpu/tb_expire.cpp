#include "tb_expire.h"

namespace tofino_cpu {

std::optional<spec_impl_t>
TBExpireFactory::speculate(const EP *ep, const Node *node,
                           const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "tb_expire") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::TofinoCPU_TokenBucket)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TBExpireFactory::process_node(const EP *ep,
                                                  const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = static_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "tb_expire") {
    return impls;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;

  addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::TofinoCPU_TokenBucket)) {
    return impls;
  }

  Module *module = new TBExpire(node, tb_addr, time);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(tb_addr,
                                         DSImpl::TofinoCPU_TokenBucket);

  return impls;
}

} // namespace tofino_cpu
