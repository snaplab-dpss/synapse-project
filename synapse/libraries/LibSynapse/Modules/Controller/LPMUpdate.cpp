#include <LibSynapse/Modules/Controller/LPMUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct lpm_update_data_t {
  addr_t obj;
  klee::ref<klee::Expr> prefix;
  klee::ref<klee::Expr> prefixlen;
  klee::ref<klee::Expr> value;
};

lpm_update_data_t get_lpm_data(const Call *lpm_update) {
  const call_t &call = lpm_update->get_call();

  klee::ref<klee::Expr> lpm_addr_expr = call.args.at("lpm").expr;
  klee::ref<klee::Expr> prefix        = call.args.at("prefix").in;
  klee::ref<klee::Expr> prefixlen     = call.args.at("prefixlen").expr;
  klee::ref<klee::Expr> value         = call.args.at("value").expr;

  const addr_t obj = expr_addr_to_obj_addr(lpm_addr_expr);

  return {obj, prefix, prefixlen, value};
}

} // namespace

std::optional<spec_impl_t> LPMUpdateFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_update = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_update->get_call();

  if (call.function_name != "lpm_update") {
    return {};
  }

  const lpm_update_data_t data = get_lpm_data(lpm_update);

  if (!speculations.ctx.can_impl_ds(data.obj, DSImpl::Controller_LPM)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> LPMUpdateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_update = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_update->get_call();

  if (call.function_name != "lpm_update") {
    return {};
  }

  const lpm_update_data_t data = get_lpm_data(lpm_update);

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Controller_LPM)) {
    return {};
  }

  Module *module  = new LPMUpdate(node, data.obj, data.prefix, data.prefixlen, data.value);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});
  new_ep->get_mutable_ctx().save_ds_impl(node->get_id(), data.obj, DSImpl::Controller_LPM);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> LPMUpdateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_update = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_update->get_call();

  if (call.function_name != "lpm_update") {
    return {};
  }

  const lpm_update_data_t data = get_lpm_data(lpm_update);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Controller_LPM)) {
    return {};
  }

  return std::make_unique<LPMUpdate>(node, data.obj, data.prefix, data.prefixlen, data.value);
}

} // namespace Controller
} // namespace LibSynapse
