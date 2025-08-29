#include <LibSynapse/Modules/Tofino/LPMLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

std::optional<spec_impl_t> LPMLookupFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_lookup = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return {};
  }

  const addr_t obj = expr_addr_to_obj_addr(call.args.at("lpm").expr);

  if (!ctx.can_impl_ds(obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  if (!can_build_lpm(ep, node, target, obj)) {
    return {};
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(obj, DSImpl::Tofino_LPM);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> LPMLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_lookup = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return {};
  }

  klee::ref<klee::Expr> lpm_addr_expr = call.args.at("lpm").expr;
  klee::ref<klee::Expr> addr          = call.args.at("prefix").expr;
  klee::ref<klee::Expr> device        = call.args.at("value_out").out;
  klee::ref<klee::Expr> match         = lpm_lookup->get_local_symbol("lpm_lookup_match").expr;

  const addr_t obj = expr_addr_to_obj_addr(lpm_addr_expr);

  if (!ep->get_ctx().can_impl_ds(obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  LPM *lpm = build_lpm(ep, node, target, obj);

  if (!lpm) {
    return {};
  }

  Module *module  = new LPMLookup(type, node, lpm->id, obj, addr, device, match);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(obj, DSImpl::Tofino_LPM);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get(), target);
  tofino_ctx->place(new_ep.get(), node, obj, lpm);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> LPMLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_lookup = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return {};
  }

  klee::ref<klee::Expr> lpm_addr_expr = call.args.at("lpm").expr;
  klee::ref<klee::Expr> addr          = call.args.at("prefix").expr;
  klee::ref<klee::Expr> device        = call.args.at("value_out").out;
  klee::ref<klee::Expr> match         = lpm_lookup->get_local_symbol("lpm_lookup_match").expr;

  const addr_t obj = expr_addr_to_obj_addr(lpm_addr_expr);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>(target)->get_data_structures().get_ds(obj);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const LPM *lpm = dynamic_cast<const LPM *>(*ds.begin());

  return std::make_unique<LPMLookup>(type, node, lpm->id, obj, addr, device, match);
}

} // namespace Tofino
} // namespace LibSynapse