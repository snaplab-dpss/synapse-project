#include <LibTessera/Modules/Controller/CMSQuery.h>
#include <LibTessera/ExecutionPlan.h>

namespace LibTessera {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> CMSQueryFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return {};
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  const addr_t cms_addr               = expr_addr_to_obj_addr(cms_addr_expr);

  if (!speculations.ctx.check_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> CMSQueryFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return {};
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const addr_t cms_addr       = expr_addr_to_obj_addr(cms_addr_expr);
  const symbol_t min_estimate = call_node->get_local_symbol("min_estimate");

  if (!ep->get_ctx().check_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return {};
  }

  Module *module  = new CMSQuery(node, cms_addr, key, min_estimate.expr);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> CMSQueryFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return {};
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  const addr_t cms_addr       = expr_addr_to_obj_addr(cms_addr_expr);
  const symbol_t min_estimate = call_node->get_local_symbol("min_estimate");

  if (!ctx.check_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return {};
  }

  return std::make_unique<CMSQuery>(node, cms_addr, key, min_estimate.expr);
}

} // namespace Controller
} // namespace LibTessera