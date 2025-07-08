#include <LibSynapse/Modules/Controller/CMSQuery.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

std::optional<spec_impl_t> CMSQueryFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
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

  if (!ctx.check_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> CMSQueryFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
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

  const addr_t cms_addr                = LibCore::expr_addr_to_obj_addr(cms_addr_expr);
  const LibCore::symbol_t min_estimate = call_node->get_local_symbol("min_estimate");

  if (!ep->get_ctx().check_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return {};
  }

  Module *module  = new CMSQuery(node, cms_addr, key, min_estimate.expr);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> CMSQueryFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
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

  const addr_t cms_addr = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.check_ds_impl(cms_addr, DSImpl::Controller_CountMinSketch)) {
    return {};
  }

  return std::make_unique<CMSQuery>(node, cms_addr, key, min_estimate.expr);
}

} // namespace Controller
} // namespace LibSynapse