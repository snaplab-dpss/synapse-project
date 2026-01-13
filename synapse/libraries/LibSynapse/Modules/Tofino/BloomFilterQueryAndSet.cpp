#include <LibSynapse/Modules/Tofino/BloomFilterQueryAndSet.h>
#include <LibSynapse/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;
using LibCore::Symbols;

namespace {

struct bf_data_t {
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> estimate;

  bf_data_t(const Context &ctx, const Call *call_node) {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "bf_bf_set");

    klee::ref<klee::Expr> bf_addr_expr = call.args.at("bf").expr;
    klee::ref<klee::Expr> key          = call.args.at("key").in;

    obj      = expr_addr_to_obj_addr(bf_addr_expr);
    keys     = Table::build_keys(key, ctx.get_expr_structs());
    estimate = call_node->get_local_symbol("bf_query_estimate").expr;
  }
};

bool is_query_and_set(const Call *bf_query, std::vector<const Call *> &bf_sets) {
  if (bf_query->get_call().function_name != "bf_query") {
    return false;
  }

  const call_t &bf_query_call = bf_query->get_call();

  klee::ref<klee::Expr> bf_addr_expr = bf_query_call.args.at("bf").expr;
  klee::ref<klee::Expr> key          = bf_query_call.args.at("key").in;

  const addr_t obj = expr_addr_to_obj_addr(bf_addr_expr);

  const std::vector<const Call *> future_bf_sets = bf_query->get_future_functions({"bf_set"});

  for (const Call *future_bf_set : future_bf_sets) {
    const call_t &bf_set = future_bf_set->get_call();

    klee::ref<klee::Expr> bf_addr_expr2 = bf_set.args.at("bf").expr;
    klee::ref<klee::Expr> key2          = bf_set.args.at("key").in;

    const addr_t obj2 = expr_addr_to_obj_addr(bf_addr_expr2);

    if ((obj == obj2) && solver_toolbox.are_exprs_always_equal(key, key2)) {
      bf_sets.push_back(future_bf_set);
    }
  }

  return !bf_sets.empty();
}

std::unique_ptr<BDD> rebuild_bdd(const EP *ep, const BDDNode *node, const std::vector<const Call *> &bf_sets, const BDDNode *&new_next_node) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *old_next_node = node->get_next();
  new_next_node                = new_bdd->get_node_by_id(old_next_node->get_id());

  Symbols symbols_to_remember;
  for (const Call *to_remove : bf_sets) {
    symbols_to_remember.add(to_remove->get_local_symbols());
    BDDNode *new_node = new_bdd->delete_non_branch(to_remove->get_id());
    if (to_remove->get_id() == old_next_node->get_id()) {
      new_next_node = new_node;
    }
  }

  return new_bdd;
}

} // namespace

std::optional<spec_impl_t> BloomFilterQueryAndSetFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *bf_query = dynamic_cast<const Call *>(node);

  std::vector<const Call *> bf_sets;
  if (!is_query_and_set(bf_query, bf_sets)) {
    return {};
  }

  const bf_data_t bf_data(ctx, bf_query);

  if (!ctx.can_impl_ds(bf_data.obj, DSImpl::Tofino_BloomFilter)) {
    return {};
  }

  const bf_config_t &cfg = ep->get_ctx().get_bf_config(bf_data.obj);

  if (!can_build_or_reuse_bf(ep, node, bf_data.obj, bf_data.keys, cfg.width, cfg.height)) {
    return {};
  }

  if (const EPNode *ep_node_leaf = ep->get_leaf_ep_node_from_bdd_node(node)) {
    if (was_ds_already_used(ep_node_leaf, build_bf_id(bf_data.obj))) {
      return {};
    }
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(bf_data.obj, DSImpl::Tofino_BloomFilter);

  spec_impl_t spec_impl = spec_impl_t(decide(ep, node), new_ctx);

  for (const Call *bf_set : bf_sets) {
    spec_impl.skip.insert(bf_set->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> BloomFilterQueryAndSetFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *bf_query = dynamic_cast<const Call *>(node);

  std::vector<const Call *> bf_sets;
  if (!is_query_and_set(bf_query, bf_sets)) {
    return {};
  }

  const bf_data_t bf_data(ep->get_ctx(), bf_query);

  if (!ep->get_ctx().can_impl_ds(bf_data.obj, DSImpl::Tofino_BloomFilter)) {
    return {};
  }

  const bf_config_t &cfg = ep->get_ctx().get_bf_config(bf_data.obj);

  BloomFilter *bf = build_or_reuse_bf(ep, node, bf_data.obj, bf_data.keys, cfg.width, cfg.height);

  if (!bf) {
    return {};
  }

  const EPNode *ep_node_leaf = ep->get_active_leaf().node;
  if (ep_node_leaf && was_ds_already_used(ep_node_leaf, bf->id)) {
    return {};
  }

  Module *module  = new BloomFilterQueryAndSet(node, bf->id, bf_data.obj, bf_data.keys, bf_data.estimate);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const BDDNode *new_next_node;
  std::unique_ptr<BDD> new_bdd = rebuild_bdd(new_ep.get(), node, bf_sets, new_next_node);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(bf_data.obj, DSImpl::Tofino_BloomFilter);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, bf_data.obj, bf);

  EPLeaf leaf(ep_node, new_next_node);
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> BloomFilterQueryAndSetFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *bf_query = dynamic_cast<const Call *>(node);

  std::vector<const Call *> bf_sets;
  if (!is_query_and_set(bf_query, bf_sets)) {
    return {};
  }

  const bf_data_t bf_data(ctx, bf_query);

  if (!ctx.check_ds_impl(bf_data.obj, DSImpl::Tofino_BloomFilter)) {
    return {};
  }

  const BloomFilter *bf = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_single_ds<BloomFilter>(bf_data.obj);

  return std::make_unique<BloomFilterQueryAndSet>(node, bf->id, bf_data.obj, bf_data.keys, bf_data.estimate);
}

} // namespace Tofino
} // namespace LibSynapse