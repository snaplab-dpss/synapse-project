#include <LibTessera/Modules/Tofino/VectorRegisterReadConditionalUpdateSingleAction.h>
#include <LibTessera/ExecutionPlan.h>

namespace LibTessera {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::is_condition_from_symbol;
using LibCore::is_increment_by_one;
using LibCore::simplify_conditional;

namespace {

vector_register_data_t get_vector_register_data(const Context &ctx, const Call *vector_borrow, const Call *vector_return) {
  const call_t &vb = vector_borrow->get_call();
  const call_t &vr = vector_return->get_call();

  const addr_t obj           = expr_addr_to_obj_addr(vb.args.at("vector").expr);
  const vector_config_t &cfg = ctx.get_vector_config(obj);

  const vector_register_data_t vector_register_data = {
      .obj         = obj,
      .capacity    = static_cast<u32>(cfg.capacity),
      .index       = vb.args.at("index").expr,
      .value       = vb.extra_vars.at("borrowed_cell").second,
      .write_value = vr.args.at("value").in,
      .actions     = {RegisterActionType::Read, RegisterActionType::Swap},
  };

  return vector_register_data;
}

std::unique_ptr<BDD> rebuild_bdd(const EP *ep, const BDDNode *node, const std::vector<const Call *> &vector_returns, const BDDNode *&new_next_node) {
  const BDD *old_bdd           = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const BDDNode *old_next_node = node->get_next();
  new_next_node                = new_bdd->get_node_by_id(old_next_node->get_id());

  for (const Call *to_remove : vector_returns) {
    BDDNode *new_node = new_bdd->delete_non_branch(to_remove->get_id());
    if (to_remove->get_id() == old_next_node->get_id()) {
      new_next_node = new_node;
    }
  }

  return new_bdd;
}

std::vector<const Call *> get_future_vector_returns(Call::vector_conditional_write_result_t &vector_conditional_write_result) {
  std::vector<const Call *> vector_returns;
  vector_returns.push_back(vector_conditional_write_result.vector_return_write);
  for (const Call *vector_return_read : vector_conditional_write_result.vector_return_reads) {
    vector_returns.push_back(vector_return_read);
  }
  return vector_returns;
}

} // namespace

std::optional<spec_impl_t> VectorRegisterReadConditionalUpdateSingleActionFactory::speculate(const EP *ep, const BDDNode *node,
                                                                                             const speculations_t &speculations) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  std::optional<Call::vector_conditional_write_result_t> vector_conditional_write_result = vector_borrow->get_vector_conditional_write_data();
  if (!vector_conditional_write_result.has_value()) {
    return {};
  }

  if (vector_conditional_write_result->conditions.size() != 1) {
    return {};
  }

  klee::ref<klee::Expr> condition = simplify_conditional(vector_conditional_write_result->conditions[0]);

  if (!is_condition_from_symbol(condition, vector_borrow->get_local_symbol("vector_data").name)) {
    return {};
  }

  const vector_register_data_t vector_register_data =
      get_vector_register_data(ep->get_ctx(), vector_borrow, vector_conditional_write_result->vector_return_write);

  const symbol_t &read_symbol = vector_borrow->get_local_symbol("vector_data");

  if (!get_tna(ep).is_simple_register_conditional_expr(vector_conditional_write_result->conditions, read_symbol)) {
    return {};
  }

  if (!is_increment_by_one(vector_register_data.value, vector_register_data.write_value)) {
    return {};
  }

  if (!speculations.ctx.can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!expr_fits_in_action(vector_register_data.write_value)) {
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

  for (const Call *vector_return : get_future_vector_returns(*vector_conditional_write_result)) {
    spec_impl.skip.insert(vector_return->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> VectorRegisterReadConditionalUpdateSingleActionFactory::process_node(const EP *ep, const BDDNode *node,
                                                                                         SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  std::optional<Call::vector_conditional_write_result_t> vector_conditional_write_result = vector_borrow->get_vector_conditional_write_data();
  if (!vector_conditional_write_result.has_value()) {
    return {};
  }

  if (vector_conditional_write_result->conditions.size() != 1) {
    return {};
  }

  klee::ref<klee::Expr> condition = simplify_conditional(vector_conditional_write_result->conditions[0]);

  if (!is_condition_from_symbol(condition, vector_borrow->get_local_symbol("vector_data").name)) {
    return {};
  }

  const vector_register_data_t vector_register_data =
      get_vector_register_data(ep->get_ctx(), vector_borrow, vector_conditional_write_result->vector_return_write);

  if (!get_tna(ep).is_simple_register_conditional_expr({condition}, vector_register_data.value)) {
    return {};
  }

  if (!is_increment_by_one(vector_register_data.value, vector_register_data.write_value)) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!expr_fits_in_action(vector_register_data.write_value)) {
    return {};
  }

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  VectorRegister *vector_register = build_or_reuse_vector_register(new_ep.get(), vector_borrow, vector_register_data);
  if (!vector_register) {
    return {};
  }

  vector_register->add_register_action(RegisterActionType::ReadConditionalWrite);

  const EPNode *ep_node_leaf = ep->get_active_leaf().node;
  if (ep_node_leaf && was_ds_already_used(ep_node_leaf, vector_register->id)) {
    return {};
  }

  Module *module =
      new VectorRegisterReadConditionalUpdateSingleAction(node, vector_register->id, vector_register_data.obj, vector_register_data.index,
                                                          vector_register_data.value, vector_register_data.write_value, condition);
  EPNode *ep_node = new EPNode(module);

  const BDDNode *new_next_node;
  std::unique_ptr<BDD> new_bdd = rebuild_bdd(new_ep.get(), node, get_future_vector_returns(*vector_conditional_write_result), new_next_node);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, vector_register_data.obj, vector_register);

  const EPLeaf leaf(ep_node, new_next_node);
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorRegisterReadConditionalUpdateSingleActionFactory::create(const BDD *bdd, const Context &ctx,
                                                                                       const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  std::optional<Call::vector_conditional_write_result_t> vector_conditional_write_result = vector_borrow->get_vector_conditional_write_data();
  if (!vector_conditional_write_result.has_value()) {
    return {};
  }

  if (vector_conditional_write_result->conditions.size() != 1) {
    return {};
  }

  klee::ref<klee::Expr> condition = simplify_conditional(vector_conditional_write_result->conditions[0]);

  if (!is_condition_from_symbol(condition, vector_borrow->get_local_symbol("vector_data").name)) {
    return {};
  }

  const vector_register_data_t vector_register_data =
      get_vector_register_data(ctx, vector_borrow, vector_conditional_write_result->vector_return_write);

  if (!ctx.get_target_ctx<TofinoContext>()->get_tna().is_simple_register_conditional_expr({condition}, vector_register_data.value)) {
    return {};
  }

  if (!ctx.check_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!expr_fits_in_action(vector_register_data.write_value)) {
    return {};
  }

  const std::unordered_set<DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(vector_register_data.obj);
  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::VectorRegister);
  VectorRegister *vector_register = dynamic_cast<VectorRegister *>(*ds.begin());

  return std::make_unique<VectorRegisterReadConditionalUpdateSingleAction>(node, vector_register->id, vector_register_data.obj,
                                                                           vector_register_data.index, vector_register_data.value,
                                                                           vector_register_data.write_value, condition);
}

} // namespace Tofino
} // namespace LibTessera