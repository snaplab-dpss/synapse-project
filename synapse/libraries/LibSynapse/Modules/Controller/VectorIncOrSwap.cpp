#include <LibSynapse/Modules/Controller/VectorIncOrSwap.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Controller {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

bool matches(const BDDNode *node) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }
  return dynamic_cast<const Call *>(node)->get_call().function_name == "vector_inc_or_swap";
}

addr_t get_vector_addr(const BDDNode *node) { return expr_addr_to_obj_addr(dynamic_cast<const Call *>(node)->get_call().args.at("vector").expr); }

std::unique_ptr<VectorIncOrSwap> build(const BDDNode *node) {
  const call_t &call = dynamic_cast<const Call *>(node)->get_call();
  return std::make_unique<VectorIncOrSwap>(node, get_vector_addr(node), call.args.at("index").expr, call.args.at("pair").in, call.args.at("pair").out,
                                           call.args.at("key_size").expr, call.args.at("value_size").expr, call.args.at("evict").expr);
}

} // namespace

std::optional<spec_impl_t> VectorIncOrSwapFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (!matches(node) || !speculations.ctx.can_impl_ds(get_vector_addr(node), DSImpl::Controller_Vector)) {
    return {};
  }
  return spec_impl_t(decide(ep, node), speculations.ctx);
}

std::vector<impl_t> VectorIncOrSwapFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (!matches(node)) {
    return {};
  }

  const addr_t vector_addr = get_vector_addr(node);
  if (!ep->get_ctx().can_impl_ds(vector_addr, DSImpl::Controller_Vector)) {
    return {};
  }

  EPNode *ep_node = new EPNode(build(node).release());

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->get_mutable_ctx().save_ds_impl(node->get_id(), vector_addr, DSImpl::Controller_Vector);

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorIncOrSwapFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (!matches(node) || !ctx.check_ds_impl(get_vector_addr(node), DSImpl::Controller_Vector)) {
    return {};
  }
  return build(node);
}

} // namespace Controller
} // namespace LibSynapse
