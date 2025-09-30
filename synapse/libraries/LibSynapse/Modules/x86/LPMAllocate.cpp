#include <LibSynapse/Modules/x86/LPMAllocate.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

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

  if (call.function_name != "lpm_allocate") {
    return false;
  }

  return true;
}
} // namespace
std::optional<spec_impl_t> LPMAllocateFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> lpm_out = call.args.at("lpm_out").out;
  symbol_t success              = call_node->get_local_symbol("lpm_alloc_success");

  // const addr_t lpm_addr = expr_addr_to_obj_addr(lpm_out);

  // if (!ctx.can_impl_ds(lpm_addr, DSImpl::x86_LPM)) {
  //   return {};
  // }

  // return spec_impl_t(decide(ep, node), ctx);
  return {};
}

std::vector<impl_t> LPMAllocateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> lpm_out = call.args.at("lpm_out").out;
  symbol_t success              = call_node->get_local_symbol("lpm_alloc_success");

  // const addr_t lpm_addr = expr_addr_to_obj_addr(lpm_out);

  // if (!ep->get_ctx().can_impl_ds(lpm_addr, DSImpl::x86_LPM)) {
  //   return {};
  // }

  Module *module  = new LPMAllocate(get_type().instance_id, node, lpm_out, success);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  // new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::x86_LPM);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return {};
}

std::unique_ptr<Module> LPMAllocateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> lpm_out = call.args.at("lpm_out").out;
  symbol_t success              = call_node->get_local_symbol("lpm_alloc_success");

  // const addr_t lpm_addr = expr_addr_to_obj_addr(lpm_out);

  // if (!ctx.check_ds_impl(lpm_addr, DSImpl::x86_LPM)) {
  //   return {};
  // }

  // return std::make_unique<LPMAllocate>(get_type().instance_id, node, lpm_out, success);
  return {};
}

} // namespace x86
} // namespace LibSynapse
