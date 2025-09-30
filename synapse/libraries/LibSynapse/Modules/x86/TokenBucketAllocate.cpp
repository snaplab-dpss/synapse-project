#include <LibSynapse/Modules/x86/TokenBucketAllocate.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>
#include <klee/util/Ref.h>

namespace LibSynapse {
namespace x86 {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
namespace {
bool bdd_node_match_pattern(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t call     = call_node->get_call();

  if (call.function_name != "tb_allocate") {
    return false;
  }

  return true;
}
} // namespace
std::optional<spec_impl_t> TokenBucketAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate     = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst    = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out   = call.args.at("tb_out").out;

  symbol_t success = call_node->get_local_symbol("tb_allocation_succeeded");

  const addr_t tb_addr = expr_addr_to_obj_addr(tb_out);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::x86_TokenBucket)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TokenBucketAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate     = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst    = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out   = call.args.at("tb_out").out;

  symbol_t success = call_node->get_local_symbol("tb_allocation_succeeded");

  const addr_t tb_addr = expr_addr_to_obj_addr(tb_out);

  if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::x86_TokenBucket)) {
    return {};
  }

  Module *module  = new TokenBucketAllocate(get_type().instance_id, node, capacity, rate, burst, key_size, tb_out, success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::x86_TokenBucket);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> TokenBucketAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate     = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst    = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out   = call.args.at("tb_out").out;

  symbol_t success = call_node->get_local_symbol("tb_allocation_succeeded");

  const addr_t tb_addr = expr_addr_to_obj_addr(tb_out);

  if (!ctx.check_ds_impl(tb_addr, DSImpl::x86_TokenBucket)) {
    return {};
  }

  return std::make_unique<TokenBucketAllocate>(get_type().instance_id, node, capacity, rate, burst, key_size, tb_out, success);
}

} // namespace x86
} // namespace LibSynapse
