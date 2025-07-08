#include <LibSynapse/Modules/Controller/TokenBucketExpire.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> TokenBucketExpireFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_expire") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  addr_t tb_addr                     = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TokenBucketExpireFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_expire") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> time         = call.args.at("time").expr;

  const addr_t tb_addr = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  Module *module  = new TokenBucketExpire(node, tb_addr, time);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::Controller_TokenBucket);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> TokenBucketExpireFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_expire") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> time         = call.args.at("time").expr;

  addr_t tb_addr = LibCore::expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  return std::make_unique<TokenBucketExpire>(node, tb_addr, time);
}

} // namespace Controller
} // namespace LibSynapse