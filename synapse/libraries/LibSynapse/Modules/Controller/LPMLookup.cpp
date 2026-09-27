#include <LibSynapse/Modules/Controller/LPMLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

struct lpm_lookup_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> match;
};

lpm_lookup_data_t get_lpm_data(const Call *lpm_lookup) {
  const call_t &call = lpm_lookup->get_call();

  klee::ref<klee::Expr> lpm_addr_expr = call.args.at("lpm").expr;
  klee::ref<klee::Expr> key           = call.args.at("prefix").in;
  klee::ref<klee::Expr> value         = call.args.at("value_out").out;
  klee::ref<klee::Expr> match         = lpm_lookup->get_local_symbol("lpm_lookup_match").expr;

  const addr_t obj = expr_addr_to_obj_addr(lpm_addr_expr);

  return {obj, key, value, match};
}

} // namespace

std::optional<spec_impl_t> LPMLookupFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *lpm_lookup = dynamic_cast<const Call *>(node);
  const call_t &call     = lpm_lookup->get_call();

  if (call.function_name != "lpm_lookup") {
    return {};
  }

  const lpm_lookup_data_t data = get_lpm_data(lpm_lookup);

  if (!speculations.ctx.can_impl_ds(data.obj, DSImpl::Controller_LPM)) {
    return {};
  }

  return spec_impl_t(decide(ep, node), speculations.ctx);
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

  const lpm_lookup_data_t data = get_lpm_data(lpm_lookup);

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Controller_LPM)) {
    return {};
  }

  Module *module  = new LPMLookup(node, data.obj, data.key, data.value, data.match);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});
  new_ep->get_mutable_ctx().save_ds_impl(node->get_id(), data.obj, DSImpl::Controller_LPM);

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

  const lpm_lookup_data_t data = get_lpm_data(lpm_lookup);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Controller_LPM)) {
    return {};
  }

  return std::make_unique<LPMLookup>(node, data.obj, data.key, data.value, data.match);
}

} // namespace Controller
} // namespace LibSynapse
