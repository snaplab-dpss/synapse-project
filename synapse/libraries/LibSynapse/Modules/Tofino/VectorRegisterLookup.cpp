#include <LibSynapse/Modules/Tofino/VectorRegisterLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

vector_register_data_t get_vector_register_data(const Context &ctx, const Call *node) {
  const call_t &call = node->get_call();

  klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index    = call.args.at("index").expr;
  klee::ref<klee::Expr> value    = call.extra_vars.at("borrowed_cell").second;

  const addr_t obj = expr_addr_to_obj_addr(obj_expr);

  const vector_config_t &cfg = ctx.get_vector_config(obj);

  const vector_register_data_t vector_register_data = {
      .obj         = obj,
      .capacity    = static_cast<u32>(cfg.capacity),
      .index       = index,
      .value       = value,
      .write_value = nullptr,
      .actions     = {RegisterActionType::Read, RegisterActionType::Swap},
  };

  return vector_register_data;
}

} // namespace

std::optional<spec_impl_t> VectorRegisterLookupFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *vector_borrow = dynamic_cast<const Call *>(node);
  const call_t &call        = vector_borrow->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  const vector_register_data_t vector_register_data = get_vector_register_data(ep->get_ctx(), vector_borrow);

  if (!ctx.can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  if (!can_build_or_reuse_vector_register(ep, vector_borrow, vector_register_data)) {
    return {};
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);
  return spec_impl;
}

std::vector<impl_t> VectorRegisterLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  const vector_register_data_t vector_register_data = get_vector_register_data(ep->get_ctx(), call_node);

  if (!ep->get_ctx().can_impl_ds(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  VectorRegister *vector_register = build_or_reuse_vector_register(ep, call_node, vector_register_data);

  if (!vector_register) {
    return {};
  }

  Module *module =
      new VectorRegisterLookup(node, vector_register->id, vector_register_data.obj, vector_register_data.index, vector_register_data.value);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, vector_register_data.obj, vector_register);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorRegisterLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  const vector_register_data_t vector_register_data = get_vector_register_data(ctx, call_node);

  if (!ctx.check_ds_impl(vector_register_data.obj, DSImpl::Tofino_VectorRegister)) {
    return {};
  }

  const std::unordered_set<DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(vector_register_data.obj);
  assert(ds.size() == 1);
  assert((*ds.begin())->type == DSType::VectorRegister);
  VectorRegister *vector_register = dynamic_cast<VectorRegister *>(*ds.begin());

  return std::make_unique<VectorRegisterLookup>(node, vector_register->id, vector_register_data.obj, vector_register_data.index,
                                                vector_register_data.value);
}

} // namespace Tofino
} // namespace LibSynapse