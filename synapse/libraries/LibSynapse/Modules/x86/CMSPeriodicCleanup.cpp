#include <LibSynapse/Modules/x86/CMSPeriodicCleanup.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

namespace {
bool bdd_node_match_pattern(const LibBDD::Node *node) {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return false;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_periodic_cleanup") {
    return false;
  }

  return true;
}
} // namespace

std::optional<spec_impl_t> CMSPeriodicCleanupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (!bdd_node_match_pattern(node)) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  addr_t cms_addr                     = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.can_impl_ds(cms_addr, DSImpl::x86_CountMinSketch)) {
    return std::nullopt;
  }

  return spec_impl_t(decide(ep, node), ctx);
}

std::vector<impl_t> CMSPeriodicCleanupFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;

  const addr_t cms_addr = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::x86_CountMinSketch)) {
    return {};
  }

  Module *module  = new CMSPeriodicCleanup(node, cms_addr);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  new_ep->get_mutable_ctx().save_ds_impl(cms_addr, DSImpl::x86_CountMinSketch);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> CMSPeriodicCleanupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (!bdd_node_match_pattern(node)) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;

  addr_t cms_addr = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.check_ds_impl(cms_addr, DSImpl::x86_CountMinSketch)) {
    return {};
  }

  return std::make_unique<CMSPeriodicCleanup>(node, cms_addr);
}

} // namespace x86
} // namespace LibSynapse