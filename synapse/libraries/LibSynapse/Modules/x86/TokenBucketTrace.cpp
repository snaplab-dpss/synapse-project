#include <LibSynapse/Modules/x86/TokenBucketTrace.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

std::optional<spec_impl_t> TokenBucketTraceFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_trace") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  addr_t tb_addr                     = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::x86_TokenBucket)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TokenBucketTraceFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_trace") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr        = call.args.at("tb").expr;
  klee::ref<klee::Expr> key                 = call.args.at("key").in;
  klee::ref<klee::Expr> pkt_len             = call.args.at("pkt_len").expr;
  klee::ref<klee::Expr> time                = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out           = call.args.at("index_out").out;
  klee::ref<klee::Expr> successfuly_tracing = call.ret;

  const addr_t tb_addr = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::x86_TokenBucket)) {
    return {};
  }

  Module *module  = new TokenBucketTrace(node, tb_addr, key, pkt_len, time, index_out, successfuly_tracing);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::x86_TokenBucket);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> TokenBucketTraceFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_trace") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr        = call.args.at("tb").expr;
  klee::ref<klee::Expr> key                 = call.args.at("key").in;
  klee::ref<klee::Expr> pkt_len             = call.args.at("pkt_len").expr;
  klee::ref<klee::Expr> time                = call.args.at("time").expr;
  klee::ref<klee::Expr> index_out           = call.args.at("index_out").out;
  klee::ref<klee::Expr> successfuly_tracing = call.ret;

  addr_t tb_addr = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.check_ds_impl(tb_addr, DSImpl::x86_TokenBucket)) {
    return {};
  }

  return std::make_unique<TokenBucketTrace>(node, tb_addr, key, pkt_len, time, index_out, successfuly_tracing);
}

} // namespace x86
} // namespace LibSynapse