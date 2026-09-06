#include <LibSynapse/Modules/Tofino/VectorRegisterReadConditionalIncrement.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Branch;
using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;
using LibCore::simplify_conditional;

namespace {

// Does `expr` contain a read of the symbol named `symbol`?
bool expr_reads_symbol(klee::ref<klee::Expr> expr, const std::string &symbol) {
  if (expr.isNull()) {
    return false;
  }
  if (expr->getKind() == klee::Expr::Read) {
    return dynamic_cast<klee::ReadExpr *>(expr.get())->updates.root->name == symbol;
  }
  for (unsigned i = 0; i < expr->getNumKids(); i++) {
    if (expr_reads_symbol(expr->getKid(i), symbol)) {
      return true;
    }
  }
  return false;
}

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
      .actions     = {RegisterActionType::Read, RegisterActionType::Write},
  };

  return vector_register_data;
}

std::vector<const Call *> get_future_vector_returns(Call::vector_conditional_write_result_t &vector_conditional_write_result) {
  std::vector<const Call *> vector_returns;
  vector_returns.push_back(vector_conditional_write_result.vector_return_write);
  for (const Call *vector_return_read : vector_conditional_write_result.vector_return_reads) {
    vector_returns.push_back(vector_return_read);
  }
  return vector_returns;
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

// The pattern this module claims: a vector_borrow whose conditional write is an
// increment of the read value (write == value + delta) gated by an EXTERNAL condition
// (not derived from the register's own value). Returns the matched pieces via outputs.
bool matches(const EP *ep, const BDDNode *node, vector_register_data_t &data, klee::ref<klee::Expr> &condition,
             std::optional<Call::vector_conditional_write_result_t> &vcw) {
  if (node->get_type() != BDDNodeType::Call) {
    return false;
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return false;
  }

  vcw = vector_borrow->get_vector_conditional_write_data();
  if (!vcw.has_value() || vcw->conditions.size() != 1) {
    return false;
  }

  condition = simplify_conditional(vcw->conditions[0]);

  // The estimator max-swap (condition from the register's own value) is handled by
  // VectorRegisterReadConditionalUpdateSingleAction; this module takes the complement.
  const std::string read_symbol = vector_borrow->get_local_symbol("vector_data").name;
  if (expr_reads_symbol(condition, read_symbol)) {
    return false;
  }

  data = get_vector_register_data(ep->get_ctx(), vector_borrow, vcw->vector_return_write);

  // The write must be an increment of the read value (value + delta); delta must not
  // reference the register's own value (a real external increment).
  std::optional<klee::ref<klee::Expr>> delta = TofinoModuleFactory::get_register_increment_delta(data.write_value, data.value);
  if (!delta.has_value()) {
    return false;
  }
  if (!TofinoModuleFactory::expr_is_materializable(*delta)) {
    return false;
  }

  // The external condition must be a gateway/action-simple comparison.
  if (!TofinoModuleFactory::get_tna(ep).is_simple_conditional_expr(condition)) {
    return false;
  }

  return true;
}

} // namespace

bool VectorRegisterReadConditionalIncrementFactory::matches_pattern(const EP *ep, const BDDNode *node) {
  vector_register_data_t data;
  klee::ref<klee::Expr> condition;
  std::optional<Call::vector_conditional_write_result_t> vcw;
  return matches(ep, node, data, condition, vcw);
}

std::optional<spec_impl_t> VectorRegisterReadConditionalIncrementFactory::speculate(const EP *ep, const BDDNode *node,
                                                                                    const speculations_t &speculations) const {
  vector_register_data_t data;
  klee::ref<klee::Expr> condition;
  std::optional<Call::vector_conditional_write_result_t> vcw;
  if (!matches(ep, node, data, condition, vcw)) {
    return {};
  }

  if (!speculations.ctx.can_impl_ds(data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  if (!can_build_or_reuse_vector_register(ep, vector_borrow, data)) {
    return {};
  }

  if (was_ds_already_used(ep->get_leaf_ep_node_from_bdd_node(node), speculations, node, data.obj, DSImpl::Tofino_VectorRegister,
                          build_vector_register_id(data.obj))) {
    return {};
  }

  Context new_ctx = speculations.ctx;
  new_ctx.save_ds_impl(node->get_id(), data.obj, DSImpl::Tofino_VectorRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  for (const Call *vector_return : get_future_vector_returns(*vcw)) {
    spec_impl.skip.insert(vector_return->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> VectorRegisterReadConditionalIncrementFactory::process_node(const EP *ep, const BDDNode *node,
                                                                                SymbolManager *symbol_manager) const {
  vector_register_data_t data;
  klee::ref<klee::Expr> condition;
  std::optional<Call::vector_conditional_write_result_t> vcw;
  if (!matches(ep, node, data, condition, vcw)) {
    return {};
  }

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  VectorRegister *vector_register = build_or_reuse_vector_register(new_ep.get(), vector_borrow, data);
  if (!vector_register) {
    return {};
  }

  const EPNode *ep_node_leaf = ep->get_active_leaf().node;
  if (ep_node_leaf && was_ds_already_used(ep_node_leaf, vector_register->id)) {
    return {};
  }

  // Keep the branch (it stays as an ordinary gateway on the external condition); only
  // the conditional-write vector_returns are absorbed into the register action.
  const BDDNode *new_next_node;
  std::unique_ptr<BDD> new_bdd = rebuild_bdd(new_ep.get(), node, get_future_vector_returns(*vcw), new_next_node);

  vector_register->add_register_action(RegisterActionType::ReadConditionalWrite);

  Module *module = new VectorRegisterReadConditionalIncrement(node, vector_register->id, data.obj, data.index, data.value, data.write_value, condition);
  EPNode *ep_node = new EPNode(module);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(node->get_id(), data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, data.obj, vector_register);

  const EPLeaf leaf(ep_node, new_next_node);
  new_ep->process_leaf(ep_node, {leaf});

  new_ep->replace_bdd(std::move(new_bdd));

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorRegisterReadConditionalIncrementFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  vector_register_data_t data;
  klee::ref<klee::Expr> condition;
  std::optional<Call::vector_conditional_write_result_t> vcw;

  // matches() needs an EP for ctx/tna; create() is only used for replay where the
  // decision was already validated, so re-run the lightweight structural checks here.
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }
  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();
  if (call.function_name != "vector_borrow") {
    return {};
  }
  vcw = vector_borrow->get_vector_conditional_write_data();
  if (!vcw.has_value() || vcw->conditions.size() != 1) {
    return {};
  }
  condition                     = simplify_conditional(vcw->conditions[0]);
  const std::string read_symbol = vector_borrow->get_local_symbol("vector_data").name;
  if (expr_reads_symbol(condition, read_symbol)) {
    return {};
  }
  data = get_vector_register_data(ctx, vector_borrow, vcw->vector_return_write);
  if (!TofinoModuleFactory::get_register_increment_delta(data.write_value, data.value).has_value()) {
    return {};
  }
  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  return std::make_unique<VectorRegisterReadConditionalIncrement>(node, build_vector_register_id(data.obj), data.obj, data.index, data.value,
                                                                  data.write_value, condition);
}

} // namespace Tofino
} // namespace LibSynapse
