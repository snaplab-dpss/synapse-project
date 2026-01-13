#include <LibSynapse/Modules/Controller/BloomFilterAllocate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> BloomFilterAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_allocate") {
    return {};
  }

  klee::ref<klee::Expr> height           = call.args.at("height").expr;
  klee::ref<klee::Expr> width            = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size         = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> bf_out           = call.args.at("bf_out").out;

  const addr_t bf_addr = expr_addr_to_obj_addr(bf_out);

  if (!ctx.can_impl_ds(bf_addr, DSImpl::Controller_BloomFilter)) {
    return {};
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(bf_addr, DSImpl::Controller_BloomFilter);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> BloomFilterAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_allocate") {
    return {};
  }

  klee::ref<klee::Expr> height           = call.args.at("height").expr;
  klee::ref<klee::Expr> width            = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size         = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> bf_out           = call.args.at("bf_out").out;

  const addr_t bf_addr = expr_addr_to_obj_addr(bf_out);

  if (!ep->get_ctx().can_impl_ds(bf_addr, DSImpl::Controller_BloomFilter)) {
    return {};
  }

  Module *module  = new BloomFilterAllocate(node, bf_addr, height, width, key_size, cleanup_interval);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_ep->get_mutable_ctx().save_ds_impl(bf_addr, DSImpl::Controller_BloomFilter);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> BloomFilterAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_allocate") {
    return {};
  }

  klee::ref<klee::Expr> height           = call.args.at("height").expr;
  klee::ref<klee::Expr> width            = call.args.at("width").expr;
  klee::ref<klee::Expr> key_size         = call.args.at("key_size").expr;
  klee::ref<klee::Expr> cleanup_interval = call.args.at("cleanup_interval").expr;
  klee::ref<klee::Expr> bf_out           = call.args.at("bf_out").out;

  const addr_t bf_addr = expr_addr_to_obj_addr(bf_out);

  if (!ctx.check_ds_impl(bf_addr, DSImpl::Controller_BloomFilter)) {
    return {};
  }

  return std::make_unique<BloomFilterAllocate>(node, bf_addr, height, width, key_size, cleanup_interval);
}

} // namespace Controller
} // namespace LibSynapse