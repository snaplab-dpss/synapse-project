#include <LibSynapse/Modules/Controller/TokenBucketIsTracing.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> TokenBucketIsTracingFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "tb_is_tracing") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  const addr_t tb_addr               = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TokenBucketIsTracingFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "tb_is_tracing") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> key          = call.args.at("key").in;
  klee::ref<klee::Expr> index_out    = call.args.at("index_out").out;
  klee::ref<klee::Expr> is_tracing   = call.ret;

  const addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  Module *module  = new TokenBucketIsTracing(node, tb_addr, key, index_out, is_tracing);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::Controller_TokenBucket);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> TokenBucketIsTracingFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "tb_is_tracing") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> key          = call.args.at("key").in;
  klee::ref<klee::Expr> index_out    = call.args.at("index_out").out;
  klee::ref<klee::Expr> is_tracing   = call.ret;

  const addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  return std::make_unique<TokenBucketIsTracing>(node, tb_addr, key, index_out, is_tracing);
}

} // namespace Controller
} // namespace LibSynapse