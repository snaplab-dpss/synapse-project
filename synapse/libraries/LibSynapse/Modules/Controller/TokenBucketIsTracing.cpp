#include <LibSynapse/Modules/Controller/TokenBucketIsTracing.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> TokenBucketIsTracingFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_is_tracing") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  addr_t tb_addr                     = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TokenBucketIsTracingFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                              LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_is_tracing") {
    return impls;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> key          = call.args.at("key").in;
  klee::ref<klee::Expr> index_out    = call.args.at("index_out").out;
  klee::ref<klee::Expr> is_tracing   = call.ret;

  addr_t tb_addr = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return impls;
  }

  Module *module  = new TokenBucketIsTracing(node, tb_addr, key, index_out, is_tracing);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::Controller_TokenBucket);

  return impls;
}

std::unique_ptr<Module> TokenBucketIsTracingFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_is_tracing") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> key          = call.args.at("key").in;
  klee::ref<klee::Expr> index_out    = call.args.at("index_out").out;
  klee::ref<klee::Expr> is_tracing   = call.ret;

  addr_t tb_addr = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  return std::make_unique<TokenBucketIsTracing>(node, tb_addr, key, index_out, is_tracing);
}

} // namespace Controller
} // namespace LibSynapse