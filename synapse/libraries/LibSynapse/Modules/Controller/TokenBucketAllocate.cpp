#include <LibSynapse/Modules/Controller/TokenBucketAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> TokenBucketAllocateFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_allocate") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate     = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst    = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out   = call.args.at("tb_out").out;

  addr_t tb_addr = LibCore::expr_addr_to_obj_addr(tb_out);

  if (!ctx.can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> TokenBucketAllocateFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                             LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_allocate") {
    return impls;
  }

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate     = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst    = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out   = call.args.at("tb_out").out;

  addr_t tb_addr = LibCore::expr_addr_to_obj_addr(tb_out);

  if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::Controller_TokenBucket)) {
    return impls;
  }

  Module *module  = new TokenBucketAllocate(node, tb_addr, capacity, rate, burst, key_size);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::Controller_TokenBucket);

  return impls;
}

std::unique_ptr<Module> TokenBucketAllocateFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "tb_allocate") {
    return {};
  }

  klee::ref<klee::Expr> capacity = call.args.at("capacity").expr;
  klee::ref<klee::Expr> rate     = call.args.at("rate").expr;
  klee::ref<klee::Expr> burst    = call.args.at("burst").expr;
  klee::ref<klee::Expr> key_size = call.args.at("key_size").expr;
  klee::ref<klee::Expr> tb_out   = call.args.at("tb_out").out;

  addr_t tb_addr = LibCore::expr_addr_to_obj_addr(tb_out);

  if (!ctx.check_ds_impl(tb_addr, DSImpl::Controller_TokenBucket)) {
    return {};
  }

  return std::make_unique<TokenBucketAllocate>(node, tb_addr, capacity, rate, burst, key_size);
}

} // namespace Controller
} // namespace LibSynapse