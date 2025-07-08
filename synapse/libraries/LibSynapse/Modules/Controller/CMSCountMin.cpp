#include <LibSynapse/Modules/Controller/CMSCountMin.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> CMSCountMinFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return std::nullopt;
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  addr_t cms_addr                     = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.can_impl_ds(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> CMSCountMinFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return {};
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  addr_t cms_addr                = LibCore::expr_addr_to_obj_addr(cms_addr_expr);
  LibCore::symbol_t min_estimate = call_node->get_local_symbol("min_estimate");

  if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return {};
  }

  Module *module  = new CMSCountMin(node, cms_addr, key, min_estimate.expr);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_ep->get_mutable_ctx().save_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> CMSCountMinFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return {};
  }

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  LibCore::symbol_t min_estimate      = call_node->get_local_symbol("min_estimate");

  addr_t cms_addr = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.check_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return {};
  }

  return std::make_unique<CMSCountMin>(node, cms_addr, key, min_estimate.expr);
}

} // namespace Controller
} // namespace LibSynapse