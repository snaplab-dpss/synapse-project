#include <LibSynapse/Modules/Tofino/CMSQuery.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

namespace {

struct cms_data_t {
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> min_estimate;

  cms_data_t(const Context &ctx, const LibBDD::Call *call_node) {
    const LibBDD::call_t &call = call_node->get_call();
    assert(call.function_name == "cms_count_min");

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    klee::ref<klee::Expr> key           = call.args.at("key").in;

    obj          = LibCore::expr_addr_to_obj_addr(cms_addr_expr);
    keys         = Table::build_keys(key, ctx.get_expr_structs());
    min_estimate = call_node->get_local_symbol("min_estimate").expr;
  }
};

} // namespace

std::optional<spec_impl_t> CMSQueryFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return std::nullopt;
  }

  const cms_data_t cms_data(ctx, call_node);

  if (!ctx.can_impl_ds(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return std::nullopt;
  }

  const LibBDD::cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_data.obj);

  if (!can_build_or_reuse_cms(ep, node, cms_data.obj, cms_data.keys, cfg.width, cfg.height)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> CMSQueryFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "cms_count_min") {
    return impls;
  }

  const cms_data_t cms_data(ep->get_ctx(), call_node);

  if (!ep->get_ctx().can_impl_ds(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return impls;
  }

  const LibBDD::cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_data.obj);

  CountMinSketch *cms = build_or_reuse_cms(ep, node, cms_data.obj, cms_data.keys, cfg.width, cfg.height);

  if (!cms) {
    return impls;
  }

  Module *module  = new CMSQuery(node, cms->id, cms_data.obj, cms_data.keys, cms_data.min_estimate);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, cms_data.obj, cms);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

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

  const cms_data_t cms_data(ctx, call_node);

  if (!ctx.check_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return {};
  }

  const CountMinSketch *cms = ctx.get_target_ctx<TofinoContext>()->get_single_ds<CountMinSketch>(cms_data.obj);

  return std::make_unique<CMSQuery>(node, cms->id, cms_data.obj, cms_data.keys, cms_data.min_estimate);
}

} // namespace Tofino
} // namespace LibSynapse