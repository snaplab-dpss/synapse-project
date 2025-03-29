#include <LibSynapse/Modules/Tofino/CMSIncAndQuery.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

namespace {
bool is_inc_and_query_cms(const LibBDD::Call *cms_increment) {
  if (cms_increment->get_call().function_name != "cms_increment") {
    return false;
  }

  klee::ref<klee::Expr> cms_addr_expr = cms_increment->get_call().args.at("cms").expr;
  addr_t cms_addr                     = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  const LibBDD::Node *next = cms_increment->get_next();

  if (!next || next->get_type() != LibBDD::NodeType::Call) {
    return false;
  }

  const LibBDD::Call *cms_count_min = dynamic_cast<const LibBDD::Call *>(next);

  if (cms_count_min->get_call().function_name != "cms_count_min") {
    return false;
  }

  klee::ref<klee::Expr> cms_addr_expr2 = cms_count_min->get_call().args.at("cms").expr;
  addr_t cms_addr2                     = LibCore::expr_addr_to_obj_addr(cms_addr_expr2);

  return cms_addr == cms_addr2;
}
} // namespace

std::optional<spec_impl_t> CMSIncAndQueryFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *cms_increment = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call        = cms_increment->get_call();

  if (!is_inc_and_query_cms(cms_increment)) {
    return std::nullopt;
  }

  const LibBDD::Call *count_min = dynamic_cast<const LibBDD::Call *>(node->get_next());

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  addr_t cms_addr = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.can_impl_ds(cms_addr, DSImpl::Tofino_CountMinSketch)) {
    return std::nullopt;
  }

  const LibBDD::cms_config_t &cfg         = ep->get_ctx().get_cms_config(cms_addr);
  std::vector<klee::ref<klee::Expr>> keys = Table::build_keys(key, ctx.get_expr_structs());

  if (!can_build_or_reuse_cms(ep, node, cms_addr, keys, cfg.width, cfg.height)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch);

  spec_impl_t spec_impl = spec_impl_t(decide(ep, node), new_ctx);
  spec_impl.skip.insert(count_min->get_id());

  return spec_impl;
}

std::vector<impl_t> CMSIncAndQueryFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                        LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *cms_increment = dynamic_cast<const LibBDD::Call *>(node);

  if (!is_inc_and_query_cms(cms_increment)) {
    return impls;
  }

  const LibBDD::Call *count_min = dynamic_cast<const LibBDD::Call *>(node->get_next());
  const LibBDD::call_t &call    = count_min->get_call();

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;

  addr_t cms_addr                = LibCore::expr_addr_to_obj_addr(cms_addr_expr);
  LibCore::symbol_t min_estimate = count_min->get_local_symbol("min_estimate");

  if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::Tofino_CountMinSketch)) {
    return impls;
  }

  const LibBDD::cms_config_t &cfg         = ep->get_ctx().get_cms_config(cms_addr);
  std::vector<klee::ref<klee::Expr>> keys = Table::build_keys(key, ep->get_ctx().get_expr_structs());

  CountMinSketch *cms = build_or_reuse_cms(ep, node, cms_addr, keys, cfg.width, cfg.height);

  if (!cms) {
    return impls;
  }

  Module *module  = new CMSIncAndQuery(node, cms->id, cms_addr, key, min_estimate.expr);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, cms_addr, cms);

  EPLeaf leaf(ep_node, count_min->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> CMSIncAndQueryFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *cms_increment = dynamic_cast<const LibBDD::Call *>(node);

  if (!is_inc_and_query_cms(cms_increment)) {
    return {};
  }

  const LibBDD::Call *count_min = dynamic_cast<const LibBDD::Call *>(node->get_next());
  const LibBDD::call_t &call    = count_min->get_call();

  klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = call.args.at("key").in;
  klee::ref<klee::Expr> min_estimate  = count_min->get_local_symbol("min_estimate").expr;

  addr_t cms_addr = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  if (!ctx.check_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch)) {
    return {};
  }

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_ds(cms_addr);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const CountMinSketch *cms = dynamic_cast<const CountMinSketch *>(*ds.begin());

  return std::make_unique<CMSIncAndQuery>(node, cms->id, cms_addr, key, min_estimate);
}

} // namespace Tofino
} // namespace LibSynapse