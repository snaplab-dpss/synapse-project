#include <LibSynapse/Modules/x86/CMSCountMin.h>
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
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> CMSCountMinFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  const addr_t cms_addr               = expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.can_impl_ds(cms_addr, DSImpl::x86_CountMinSketch)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> CMSCountMinFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const addr_t cms_addr       = expr_addr_to_obj_addr(cms_addr_expr);
  const symbol_t min_estimate = call_node->get_local_symbol("min_estimate");

  if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::x86_CountMinSketch)) {
    return {};
  }

  Module *module  = new CMSCountMin(type, node, cms_addr, key, min_estimate.expr);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_ep->get_mutable_ctx().save_ds_impl(cms_addr, DSImpl::x86_CountMinSketch);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> CMSCountMinFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const addr_t cms_addr       = expr_addr_to_obj_addr(cms_addr_expr);
  const symbol_t min_estimate = call_node->get_local_symbol("min_estimate");

  if (!ctx.check_ds_impl(cms_addr, DSImpl::x86_CountMinSketch)) {
    return {};
  }

  return std::make_unique<CMSCountMin>(type, node, cms_addr, key, min_estimate.expr);
}

} // namespace x86
} // namespace LibSynapse