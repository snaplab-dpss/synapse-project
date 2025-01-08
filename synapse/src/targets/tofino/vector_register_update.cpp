#include "vector_register_update.h"

namespace synapse {
namespace tofino {
namespace {
vector_register_data_t get_vector_register_data(const EP *ep, const Call *vector_borrow,
                                                const Call *vector_return) {
  const call_t &vb = vector_borrow->get_call();
  const call_t &vr = vector_return->get_call();

  addr_t obj = expr_addr_to_obj_addr(vb.args.at("vector").expr);
  const vector_config_t &cfg = ep->get_ctx().get_vector_config(obj);

  vector_register_data_t vector_register_data = {
      .obj = obj,
      .num_entries = static_cast<u32>(cfg.capacity),
      .index = vb.args.at("index").expr,
      .value = vb.extra_vars.at("borrowed_cell").second,
      .write_value = vr.args.at("value").in,
      .actions = {RegisterActionType::READ, RegisterActionType::SWAP},
  };

  return vector_register_data;
}

} // namespace

std::optional<spec_impl_t> VectorRegisterUpdateFactory::speculate(const EP *ep, const Node *node,
                                                                  const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *vector_return = dynamic_cast<const Call *>(node);
  const call_t &call = vector_return->get_call();

  if (call.function_name != "vector_return") {
    return std::nullopt;
  }

  if (!vector_return->is_vector_write()) {
    return std::nullopt;
  }

  const Call *vector_borrow = vector_return->get_vector_borrow_from_return();

  vector_register_data_t vector_register_data =
      get_vector_register_data(ep, vector_borrow, vector_return);

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
  return spec_impl;
}

std::vector<impl_t> VectorRegisterUpdateFactory::process_node(const EP *ep, const Node *node,
                                                              SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *vector_return = dynamic_cast<const Call *>(node);
  const call_t &call = vector_return->get_call();

  if (call.function_name != "vector_return") {
    return impls;
  }

  if (!vector_return->is_vector_write()) {
    return impls;
  }

  const Call *vector_borrow = vector_return->get_vector_borrow_from_return();

  vector_register_data_t vector_register_data =
      get_vector_register_data(ep, vector_borrow, vector_return);

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
                               vector_register_data.value, vector_register_data.write_value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  for (Register *reg : regs) {
    tofino_ctx->place(new_ep, node, vector_register_data.obj, reg);
  }

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

} // namespace tofino
} // namespace synapse