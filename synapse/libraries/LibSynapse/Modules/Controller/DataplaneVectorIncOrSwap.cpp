#include <LibSynapse/Modules/Controller/DataplaneVectorIncOrSwap.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::solver_toolbox;

namespace {

bool matches(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  return dynamic_cast<const Call *>(node)->get_call().function_name == "vector_inc_or_swap";
}

addr_t get_obj(const BDDNode *node) { return expr_addr_to_obj_addr(dynamic_cast<const Call *>(node)->get_call().args.at("vector").expr); }

std::unique_ptr<DataplaneVectorIncOrSwap> build(const BDDNode *node) {
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return std::make_unique<DataplaneVectorIncOrSwap>(node, get_obj(node), call.args.at("index").expr, call.args.at("pair").in,
                                                    call.args.at("pair").out,
                                                    static_cast<bits_t>(solver_toolbox.value_from_expr(call.args.at("key_size").expr) * 8),
                                                    solver_toolbox.value_from_expr(call.args.at("evict").expr) != 0);
}

} // namespace

std::optional<spec_impl_t> DataplaneVectorIncOrSwapFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!matches(node) || !speculations.ctx.check_ds_impl(get_obj(node), DSImpl::Tofino_VectorRegister)) {
    return {};
  }
  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> DataplaneVectorIncOrSwapFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!matches(node) || !ep->get_ctx().check_ds_impl(get_obj(node), DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  EPNode *ep_node = new EPNode(build(node).release());

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> DataplaneVectorIncOrSwapFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!matches(node) || !ctx.check_ds_impl(get_obj(node), DSImpl::Tofino_VectorRegister)) {
    return {};
  }
  return build(node);
}

} // namespace Controller
} // namespace LibSynapse
