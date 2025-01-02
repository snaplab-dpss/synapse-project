#include "tb_trace.h"

namespace synapse {
namespace controller {

std::optional<spec_impl_t> TBTraceFactory::speculate(const EP *ep, const Node *node,
                                                     const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "tb_trace") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TBTraceFactory::process_node(const EP *ep, const Node *node) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call = call_node->get_call();

  if (call.function_name != "tb_trace") {
    return impls;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> key = call.args.at("key").in;
  klee::ref<klee::Expr> pkt_len = call.args.at("pkt_len").expr;
  klee::ref<klee::Expr> time = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out = call.args.at("index_out").out;
  klee::ref<klee::Expr> successfuly_tracing = call.ret;

  addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return impls;
  }

  Module *module =
      new TBTrace(node, tb_addr, key, pkt_len, time, index_out, successfuly_tracing);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::Controller_TokenBucket);

  return impls;
}

} // namespace controller
} // namespace synapse