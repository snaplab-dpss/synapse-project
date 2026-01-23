#include <LibSynapse/Modules/Controller/BloomFilterQuery.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> BloomFilterQueryFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_query") {
    return {};
  }

  klee::ref<klee::Expr> bf_addr_expr = call.args.at("bf").expr;
  const addr_t bf_addr               = expr_addr_to_obj_addr(bf_addr_expr);

  if (!speculations.ctx.can_impl_ds(bf_addr, DSImpl::Controller_BloomFilter)) {
    return {};
  }

  Context new_ctx = speculations.ctx;
  new_ctx.save_ds_impl(node->get_id(), bf_addr, DSImpl::Controller_BloomFilter);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> BloomFilterQueryFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_query") {
    return {};
  }

  klee::ref<klee::Expr> bf_addr_expr = call.args.at("bf").expr;
  klee::ref<klee::Expr> key          = call.args.at("key").in;

  const addr_t bf_addr    = expr_addr_to_obj_addr(bf_addr_expr);
  const symbol_t estimate = call_node->get_local_symbol("bf_query_estimate");

  if (!ep->get_ctx().can_impl_ds(bf_addr, DSImpl::Controller_BloomFilter)) {
    return {};
  }

  Module *module  = new BloomFilterQuery(node, bf_addr, key, estimate.expr);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_ep->get_mutable_ctx().save_ds_impl(node->get_id(), bf_addr, DSImpl::Controller_BloomFilter);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> BloomFilterQueryFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_query") {
    return {};
  }

  klee::ref<klee::Expr> bf_addr_expr = call.args.at("bf").expr;
  klee::ref<klee::Expr> key          = call.args.at("key").in;

  const addr_t bf_addr    = expr_addr_to_obj_addr(bf_addr_expr);
  const symbol_t estimate = call_node->get_local_symbol("bf_query_estimate");

  if (!ctx.check_ds_impl(bf_addr, DSImpl::Controller_BloomFilter)) {
    return {};
  }

  return std::make_unique<BloomFilterQuery>(node, bf_addr, key, estimate.expr);
}

} // namespace Controller
} // namespace LibSynapse