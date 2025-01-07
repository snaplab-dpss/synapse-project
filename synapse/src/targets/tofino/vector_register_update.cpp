#include "vector_register_update.h"

namespace synapse {
namespace tofino {
namespace {
bool is_conditional_write(const Call *node, const Call *&vector_return) {
  vector_return = node->get_vector_return_from_borrow();
  return false;
}

vector_register_data_t get_vector_register_data(const EP *ep, const Call *vector_borrow,
                                                const Call *vector_return,
                                                std::vector<mod_t> &changes) {
  const call_t &call = vector_borrow->get_call();

  klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index = call.args.at("index").expr;
  klee::ref<klee::Expr> value = call.extra_vars.at("borrowed_cell").second;

  addr_t obj = expr_addr_to_obj_addr(obj_expr);
  changes = build_vector_modifications(vector_borrow, vector_return);

  const Context &ctx = ep->get_ctx();
  const vector_config_t &cfg = ctx.get_vector_config(obj);

  vector_register_data_t vector_register_data = {
      .obj = obj,
      .num_entries = static_cast<u32>(cfg.capacity),
      .index = index,
      .value = value,
      .actions = {RegisterAction::READ, RegisterAction::SWAP},
  };

  return vector_register_data;
}

std::unique_ptr<BDD> delete_future_vector_return(EP *ep, const Node *node,
                                                 const Node *vector_return, const Node *&new_next) {
  const BDD *old_bdd = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const Node *next = node->get_next();

  if (next) {
    new_next = new_bdd->get_node_by_id(next->get_id());
  } else {
    new_next = nullptr;
  }

  bool replace_next = (vector_return == next);
  Node *replacement = new_bdd->delete_non_branch(vector_return->get_id());

  if (replace_next) {
    new_next = replacement;
  }

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> VectorRegisterUpdateFactory::speculate(const EP *ep, const Node *node,
                                                                  const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return std::nullopt;
  }

  if (vector_borrow->is_vector_read()) {
    return std::nullopt;
  }

  const Call *vector_return;
  if (is_conditional_write(vector_borrow, vector_return)) {
    return std::nullopt;
  }

  std::vector<mod_t> changes;
  vector_register_data_t vector_register_data =
      get_vector_register_data(ep, vector_borrow, vector_return, changes);

  // This will be ignored by the Ignore module.
  if (changes.empty()) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return std::nullopt;
  }

  bool can_place_ds = can_build_or_reuse_vector_registers(ep, vector_borrow, vector_register_data);

  if (!can_place_ds) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  spec_impl.skip.insert(vector_return->get_id());

  return spec_impl;
}

std::vector<impl_t> VectorRegisterUpdateFactory::process_node(const EP *ep, const Node *node,
                                                              SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return impls;
  }

  if (vector_borrow->is_vector_read()) {
    return impls;
  }

  const Call *vector_return;
  if (is_conditional_write(vector_borrow, vector_return)) {
    return impls;
  }

  std::vector<mod_t> changes;
  vector_register_data_t vector_register_data =
      get_vector_register_data(ep, vector_borrow, vector_return, changes);

  // Check the Ignore module.
  if (changes.empty()) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return impls;
  }

  std::unordered_set<Register *> regs =
      build_or_reuse_vector_registers(ep, vector_borrow, vector_register_data);

  if (regs.empty()) {
    return impls;
  }

  std::unordered_set<DS_ID> rids;
  for (DS *reg : regs) {
    rids.insert(reg->id);
  }

  Module *module =
      new VectorRegisterUpdate(node, rids, vector_register_data.obj, vector_register_data.index,
                               vector_register_data.value, changes);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  const Node *new_next;
  std::unique_ptr<BDD> new_bdd = delete_future_vector_return(new_ep, node, vector_return, new_next);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);

  for (Register *reg : regs) {
    tofino_ctx->place(new_ep, node, vector_register_data.obj, reg);
  }

  EPLeaf leaf(ep_node, new_next);
  new_ep->process_leaf(ep_node, {leaf});
  new_ep->replace_bdd(std::move(new_bdd));
  new_ep->assert_integrity();

  return impls;
}

} // namespace tofino
} // namespace synapse