#include "vector_register_lookup.hpp"

namespace synapse {
namespace tofino {
namespace {
vector_register_data_t get_vector_register_data(const EP *ep, const Call *node) {

  const call_t &call = node->get_call();

  klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index    = call.args.at("index").expr;
  klee::ref<klee::Expr> value    = call.extra_vars.at("borrowed_cell").second;

  addr_t obj = expr_addr_to_obj_addr(obj_expr);

  const Context &ctx         = ep->get_ctx();
  const vector_config_t &cfg = ctx.get_vector_config(obj);

  vector_register_data_t vector_register_data = {
      .obj         = obj,
      .num_entries = static_cast<u32>(cfg.capacity),
      .index       = index,
      .value       = value,
      .write_value = nullptr,
      .actions     = {RegisterActionType::READ, RegisterActionType::SWAP},
  };

  return vector_register_data;
}
} // namespace

std::optional<spec_impl_t> VectorRegisterLookupFactory::speculate(const EP *ep, const Node *node,
                                                                  const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return std::nullopt;
  }

  vector_register_data_t vector_register_data = get_vector_register_data(ep, vector_borrow);

  if (!ctx.can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return std::nullopt;
  }

  if (!can_build_or_reuse_vector_registers(ep, vector_borrow, vector_register_data)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  return spec_impl;
}

std::vector<impl_t> VectorRegisterLookupFactory::process_node(const EP *ep, const Node *node,
                                                              SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return impls;
  }

  vector_register_data_t vector_register_data = get_vector_register_data(ep, call_node);

  if (!ep->get_ctx().can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return impls;
  }

  std::unordered_set<Register *> regs = build_or_reuse_vector_registers(ep, call_node, vector_register_data);

  if (regs.empty()) {
    return impls;
  }

  std::unordered_set<DS_ID> rids;
  for (DS *reg : regs) {
    rids.insert(reg->id);
  }

  Module *module  = new VectorRegisterLookup(node, rids, vector_register_data.obj, vector_register_data.index,
                                             vector_register_data.value);
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