#include <LibSynapse/Modules/Tofino/LPMLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

std::optional<spec_impl_t> LPMLookupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *lpm_lookup = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return std::nullopt;
  }

  addr_t obj = LibCore::expr_addr_to_obj_addr(call.args.at("lpm").expr);

  if (!ctx.can_impl_ds(obj, DSImpl::Tofino_LPM)) {
    return std::nullopt;
  }

  if (!can_build_lpm(ep, node, obj)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(obj, DSImpl::Tofino_LPM);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> LPMLookupFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *lpm_lookup = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return impls;
  }

  klee::ref<klee::Expr> lpm_addr_expr = call.args.at("lpm").expr;
  klee::ref<klee::Expr> addr          = call.args.at("prefix").expr;
  klee::ref<klee::Expr> device        = call.args.at("value_out").out;
  klee::ref<klee::Expr> match         = lpm_lookup->get_local_symbol("lpm_lookup_match").expr;

  addr_t obj = LibCore::expr_addr_to_obj_addr(lpm_addr_expr);

  if (!ep->get_ctx().can_impl_ds(obj, DSImpl::Tofino_LPM)) {
    return impls;
  }

  LPM *lpm = build_lpm(ep, node, obj);

  if (!lpm) {
    return impls;
  }

  Module *module  = new LPMLookup(node, lpm->id, obj, addr, device, match);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(obj, DSImpl::Tofino_LPM);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, obj, lpm);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> LPMLookupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *lpm_lookup = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return {};
  }

  klee::ref<klee::Expr> lpm_addr_expr = call.args.at("lpm").expr;
  klee::ref<klee::Expr> addr          = call.args.at("prefix").expr;
  klee::ref<klee::Expr> device        = call.args.at("value_out").out;
  klee::ref<klee::Expr> match         = lpm_lookup->get_local_symbol("lpm_lookup_match").expr;

  addr_t obj = LibCore::expr_addr_to_obj_addr(lpm_addr_expr);

  if (!ctx.check_ds_impl(obj, DSImpl::Tofino_LPM)) {
    return {};
  }

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_ds(obj);
  assert(ds.size() == 1 && "Expected exactly one DS");
  const LPM *lpm = dynamic_cast<const LPM *>(*ds.begin());

  return std::make_unique<LPMLookup>(node, lpm->id, obj, addr, device, match);
}

} // namespace Tofino
} // namespace LibSynapse