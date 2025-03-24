#include <LibSynapse/Modules/Tofino/VectorRegisterLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {

vector_register_data_t get_vector_register_data(const Context &ctx, const LibBDD::Call *node) {
  const LibBDD::call_t &call = node->get_call();

  klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index    = call.args.at("index").expr;
  klee::ref<klee::Expr> value    = call.extra_vars.at("borrowed_cell").second;

  addr_t obj = LibCore::expr_addr_to_obj_addr(obj_expr);

  const LibBDD::vector_config_t &cfg = ctx.get_vector_config(obj);

  vector_register_data_t vector_register_data = {
      .obj         = obj,
      .capacity    = static_cast<u32>(cfg.capacity),
      .index       = index,
      .value       = value,
      .write_value = nullptr,
      .actions     = {RegisterActionType::READ, RegisterActionType::SWAP},
  };

  return vector_register_data;
}

} // namespace

std::optional<spec_impl_t> VectorRegisterLookupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *vector_borrow = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return std::nullopt;
  }

  vector_register_data_t vector_register_data = get_vector_register_data(ep->get_ctx(), vector_borrow);

  if (!ctx.can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return std::nullopt;
  }

  if (!can_build_or_reuse_vector_register(ep, vector_borrow, vector_register_data)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  return spec_impl;
}

std::vector<impl_t> VectorRegisterLookupFactory::process_node(const EP *ep, const LibBDD::Node *node,
                                                              LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return impls;
  }

  // If the value is ignored everywhere but in the header modification, we will transpile it to a vector lookup during the header
  // modification.
  const bool can_be_inlined = call_node->get_vector_borrow_value_future_usage() <= 1;

  vector_register_data_t vector_register_data = get_vector_register_data(ep->get_ctx(), call_node);

  if (!ep->get_ctx().can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return impls;
  }

  VectorRegister *vector_register = build_or_reuse_vector_register(ep, call_node, vector_register_data);

  if (!vector_register) {
    return impls;
  }

  Module *module  = new VectorRegisterLookup(node, vector_register->id, vector_register_data.obj, vector_register_data.index,
                                             vector_register_data.value, can_be_inlined);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, vector_register_data.obj, vector_register);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> VectorRegisterLookupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  vector_register_data_t vector_register_data = get_vector_register_data(ctx, call_node);

  if (!ctx.check_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  // If the value is ignored everywhere but in the header modification, we will transpile it to a vector lookup during the header
  // modification.
  const bool can_be_inlined = call_node->get_vector_borrow_value_future_usage() <= 1;

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_ds(vector_register_data.obj);
  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::VECTOR_REGISTER);
  Tofino::VectorRegister *vector_register = dynamic_cast<Tofino::VectorRegister *>(*ds.begin());

  return std::make_unique<VectorRegisterLookup>(node, vector_register->id, vector_register_data.obj, vector_register_data.index,
                                                vector_register_data.value, can_be_inlined);
}

} // namespace Tofino
} // namespace LibSynapse