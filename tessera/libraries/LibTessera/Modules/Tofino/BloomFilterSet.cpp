#include <LibTessera/Modules/Tofino/BloomFilterSet.h>
#include <LibTessera/ExecutionPlan.h>
#include <LibCore/Expr.h>

namespace LibTessera {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct bf_data_t {
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;

  bf_data_t(const Context &ctx, const Call *call_node) {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "bf_set");

    klee::ref<klee::Expr> obj_expr = call.args.at("bf").expr;
    klee::ref<klee::Expr> key      = call.args.at("key").in;

    obj  = expr_addr_to_obj_addr(obj_expr);
    keys = Table::build_keys(key, ctx.get_expr_structs());
  }
};

} // namespace

std::optional<spec_impl_t> BloomFilterSetFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_set") {
    return {};
  }

  const bf_data_t bf_data(speculations.ctx, call_node);

  if (!speculations.ctx.can_impl_ds(bf_data.obj, DSImpl::Tofino_BloomFilter)) {
    return {};
  }

  const bf_config_t &cfg = ep->get_ctx().get_bf_config(bf_data.obj);

  if (!can_build_or_reuse_bf(ep, node, bf_data.obj, bf_data.keys, cfg.width, cfg.height)) {
    return {};
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), speculations, node, bf_data.obj, DSImpl::Tofino_BloomFilter,
                          build_bf_id(bf_data.obj))) {
    return {};
  }

  Context new_ctx = speculations.ctx;
  new_ctx.save_ds_impl(node->get_id(), bf_data.obj, DSImpl::Tofino_BloomFilter);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> BloomFilterSetFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_set") {
    return {};
  }

  const bf_data_t bf_data(ep->get_ctx(), call_node);

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

  Module *module  = new BloomFilterSet(node, bf->id, bf_data.obj, bf_data.keys);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), bf_data.obj, DSImpl::Tofino_BloomFilter);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, bf_data.obj, bf);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> BloomFilterSetFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "bf_set") {
    return {};
  }

  const bf_data_t bf_data(ctx, call_node);

  if (!ctx.check_ds_impl(bf_data.obj, DSImpl::Tofino_BloomFilter)) {
    return {};
  }

  const BloomFilter *bf = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_single_ds<BloomFilter>(bf_data.obj);

  return std::make_unique<BloomFilterSet>(node, bf->id, bf_data.obj, bf_data.keys);
}

} // namespace Tofino
} // namespace LibTessera