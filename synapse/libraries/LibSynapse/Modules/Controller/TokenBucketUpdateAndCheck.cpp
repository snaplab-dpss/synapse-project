#include <LibSynapse/Modules/Controller/TokenBucketUpdateAndCheck.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> TokenBucketUpdateAndCheckFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "tb_update_and_check") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  const addr_t tb_addr               = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TokenBucketUpdateAndCheckFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "tb_update_and_check") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> index        = call.args.at("index").expr;
  klee::ref<klee::Expr> pkt_len      = call.args.at("pkt_len").expr;
  klee::ref<klee::Expr> time         = call.args.at("time").expr;
  klee::ref<klee::Expr> pass         = call.ret;

  const addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  Module *module  = new TokenBucketUpdateAndCheck(node, tb_addr, index, pkt_len, time, pass);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::Controller_TokenBucket);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> TokenBucketUpdateAndCheckFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "tb_update_and_check") {
    return {};
  }

  klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
  klee::ref<klee::Expr> index        = call.args.at("index").expr;
  klee::ref<klee::Expr> pkt_len      = call.args.at("pkt_len").expr;
  klee::ref<klee::Expr> time         = call.args.at("time").expr;
  klee::ref<klee::Expr> pass         = call.ret;

  const addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

  if (!ctx.check_ds_impl(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  return std::make_unique<TokenBucketUpdateAndCheck>(node, tb_addr, index, pkt_len, time, pass);
}

} // namespace Controller
} // namespace LibSynapse