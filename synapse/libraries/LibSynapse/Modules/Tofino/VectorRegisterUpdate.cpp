#include <LibSynapse/Modules/Tofino/VectorRegisterUpdate.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {
vector_register_data_t get_vector_register_data(const Context &ctx, const Call *vector_borrow, const Call *vector_return) {
  const call_t &vb = vector_borrow->get_call();
  const call_t &vr = vector_return->get_call();

  const addr_t obj           = expr_addr_to_obj_addr(vb.args.at("vector").expr);
  const vector_config_t &cfg = ctx.get_vector_config(obj);

  klee::ref<klee::Expr> read_value  = vb.extra_vars.at("borrowed_cell").second;
  klee::ref<klee::Expr> write_value = vr.args.at("value").in;

  // `*reg += delta` is lowered to a stateful-ALU increment (AddValue); a plain
  // overwrite `*reg = value` is a Write.
  const bool is_increment = TofinoModuleFactory::get_register_increment_delta(write_value, read_value).has_value();

  const vector_register_data_t vector_register_data = {
      .obj         = obj,
      .capacity    = static_cast<u32>(cfg.capacity),
      .index       = vb.args.at("index").expr,
      .value       = read_value,
      .write_value = write_value,
      .actions     = {RegisterActionType::Read, is_increment ? RegisterActionType::AddValue : RegisterActionType::Write},
  };

  return vector_register_data;
}

// The write value fits either directly (overwrite by a single external value) or
// as an increment whose delta is computable in a regular action (<=2 symbols).
bool write_value_is_compatible(const vector_register_data_t &d) {
  if (TofinoModuleFactory::expr_fits_in_action(d.write_value)) {
    return true;
  }
  std::optional<klee::ref<klee::Expr>> delta = TofinoModuleFactory::get_register_increment_delta(d.write_value, d.value);
  return delta.has_value() && TofinoModuleFactory::expr_fits_in_action(*delta, 2);
}

} // namespace

std::optional<spec_impl_t> VectorRegisterUpdateFactory::speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_return = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_return->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  if (!vector_return->is_vector_write()) {
    return {};
  }

  const Call *vector_borrow = vector_return->get_vector_borrow_from_return();

  const vector_register_data_t vector_register_data = get_vector_register_data(ep->get_ctx(), vector_borrow, vector_return);

  if (!speculations.ctx.can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!write_value_is_compatible(vector_register_data)) {
    return {};
  }

  if (!can_build_or_reuse_vector_register(ep, vector_borrow, vector_register_data)) {
    return {};
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), speculations, node, vector_register_data.obj, DSImpl::Tofino_VectorRegister,
                          build_vector_register_id(vector_register_data.obj))) {
    return {};
  }

  Context new_ctx = speculations.ctx;
  new_ctx.save_ds_impl(node->get_id(), vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  return spec_impl;
}

std::vector<impl_t> VectorRegisterUpdateFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_return = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_return->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  if (!vector_return->is_vector_write()) {
    return {};
  }

  const Call *vector_borrow                         = vector_return->get_vector_borrow_from_return();
  const vector_register_data_t vector_register_data = get_vector_register_data(ep->get_ctx(), vector_borrow, vector_return);

  if (!ep->get_ctx().can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!write_value_is_compatible(vector_register_data)) {
    return {};
  }

  VectorRegister *vector_register = build_or_reuse_vector_register(ep, vector_borrow, vector_register_data);

  if (!vector_register) {
    return {};
  }

  const EPNode *ep_node_leaf = ep->get_active_leaf().node;
  if (ep_node_leaf && was_ds_already_used(ep_node_leaf, vector_register->id)) {
    return {};
  }

  Module *module  = new VectorRegisterUpdate(node, vector_register->id, vector_register_data.obj, vector_register_data.index,
                                             vector_register_data.value, vector_register_data.write_value);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, vector_register_data.obj, vector_register);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorRegisterUpdateFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_return = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_return->get_call();

  if (call.function_name != "vector_return") {
    return {};
  }

  if (!vector_return->is_vector_write()) {
    return {};
  }

  const Call *vector_borrow                         = vector_return->get_vector_borrow_from_return();
  const vector_register_data_t vector_register_data = get_vector_register_data(ctx, vector_borrow, vector_return);

  if (!ctx.check_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!write_value_is_compatible(vector_register_data)) {
    return {};
  }

  const std::unordered_set<DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(vector_register_data.obj);
  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::VectorRegister);
  VectorRegister *vector_register = dynamic_cast<VectorRegister *>(*ds.begin());

  return std::make_unique<VectorRegisterUpdate>(node, vector_register->id, vector_register_data.obj, vector_register_data.index,
                                                vector_register_data.value, vector_register_data.write_value);
}

} // namespace Tofino
} // namespace LibSynapse