#include <LibSynapse/Modules/Tofino/CMSIncAndQuery.h>
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

bool is_inc_and_query_cms(const LibBDD::Call *cms_increment, std::vector<const LibBDD::Call *> &cms_count_mins) {
  if (cms_increment->get_call().function_name != "cms_increment") {
    return false;
  }

  const LibBDD::call_t &cms_increment_call = cms_increment->get_call();

  klee::ref<klee::Expr> cms_addr_expr = cms_increment_call.args.at("cms").expr;
  klee::ref<klee::Expr> key           = cms_increment_call.args.at("key").in;

  const addr_t obj = LibCore::expr_addr_to_obj_addr(cms_addr_expr);

  const std::vector<const LibBDD::Call *> future_cms_count_mins = cms_increment->get_future_functions({"cms_count_min"});

  for (const LibBDD::Call *future_cms_count_min : future_cms_count_mins) {
    const LibBDD::call_t &cms_count_min_call = future_cms_count_min->get_call();

    klee::ref<klee::Expr> cms_addr_expr2 = cms_count_min_call.args.at("cms").expr;
    klee::ref<klee::Expr> key2           = cms_count_min_call.args.at("key").in;

    const addr_t obj2 = LibCore::expr_addr_to_obj_addr(cms_addr_expr2);

    if ((obj == obj2) && LibCore::solver_toolbox.are_exprs_always_equal(key, key2)) {
      cms_count_mins.push_back(future_cms_count_min);
    }
  }

  return !cms_count_mins.empty();
}

std::unique_ptr<LibBDD::BDD> rebuild_bdd(const EP *ep, const LibBDD::Node *node, const std::vector<const LibBDD::Call *> &cms_count_mins,
                                         const LibBDD::Node *&new_next_node) {
  const LibBDD::BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<LibBDD::BDD> new_bdd = std::make_unique<LibBDD::BDD>(*old_bdd);

  const LibBDD::Node *old_next_node = node->get_next();
  new_next_node                     = new_bdd->get_node_by_id(old_next_node->get_id());

  LibCore::Symbols symbols_to_remember;
  for (const LibBDD::Call *to_remove : cms_count_mins) {
    symbols_to_remember.add(to_remove->get_local_symbols());
    LibBDD::Node *new_node = new_bdd->delete_non_branch(to_remove->get_id());
    if (to_remove->get_id() == old_next_node->get_id()) {
      new_next_node = new_node;
    }
  }

  new_bdd->add_new_symbol_generator_function(new_next_node->get_id(), "cms_inc_and_query_min_estimate", symbols_to_remember);

  return new_bdd;
}

} // namespace

std::optional<spec_impl_t> CMSIncAndQueryFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *cms_increment = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> cms_count_mins;
  if (!is_inc_and_query_cms(cms_increment, cms_count_mins)) {
    return std::nullopt;
  }

  const cms_data_t cms_data(ctx, cms_count_mins[0]);

  if (!ctx.can_impl_ds(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return std::nullopt;
  }

  const LibBDD::cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_data.obj);

  if (!can_build_or_reuse_cms(ep, node, cms_data.obj, cms_data.keys, cfg.width, cfg.height)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch);

  spec_impl_t spec_impl = spec_impl_t(decide(ep, node), new_ctx);

  for (const LibBDD::Call *count_min : cms_count_mins) {
    spec_impl.skip.insert(count_min->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> CMSIncAndQueryFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *cms_increment = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> cms_count_mins;
  if (!is_inc_and_query_cms(cms_increment, cms_count_mins)) {
    return {};
  }

  const cms_data_t cms_data(ep->get_ctx(), cms_count_mins[0]);

  if (!ep->get_ctx().can_impl_ds(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return {};
  }

  const LibBDD::cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_data.obj);

  CountMinSketch *cms = build_or_reuse_cms(ep, node, cms_data.obj, cms_data.keys, cfg.width, cfg.height);

  if (!cms) {
    return {};
  }

  Module *module  = new CMSIncAndQuery(node, cms->id, cms_data.obj, cms_data.keys, cms_data.min_estimate);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const LibBDD::Node *new_next_node;
  std::unique_ptr<LibBDD::BDD> new_bdd = rebuild_bdd(new_ep.get(), node, cms_count_mins, new_next_node);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, cms_data.obj, cms);

  EPLeaf leaf(ep_node, new_next_node);
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->replace_bdd(std::move(new_bdd));
  new_ep->assert_integrity();

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> CMSIncAndQueryFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *cms_increment = dynamic_cast<const LibBDD::Call *>(node);

  std::vector<const LibBDD::Call *> cms_count_mins;
  if (!is_inc_and_query_cms(cms_increment, cms_count_mins)) {
    return {};
  }

  const cms_data_t cms_data(ctx, cms_count_mins[0]);

  if (!ctx.check_ds_impl(cms_data.obj, DSImpl::Tofino_CountMinSketch)) {
    return {};
  }

  const CountMinSketch *cms = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_single_ds<CountMinSketch>(cms_data.obj);

  return std::make_unique<CMSIncAndQuery>(node, cms->id, cms_data.obj, cms_data.keys, cms_data.min_estimate);
}

} // namespace Tofino
} // namespace LibSynapse