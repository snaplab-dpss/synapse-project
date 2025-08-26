#include <LibSynapse/Modules/Tofino/VectorTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

using LibCore::expr_addr_to_obj_addr;

namespace {

vector_table_data_t get_vector_table_data(const Context &ctx, const Call *call_node) {
  const call_t &call = call_node->get_call();
  assert(call.function_name == "vector_borrow" && "Unexpected function");

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> cell             = call.extra_vars.at("borrowed_cell").second;

  const addr_t obj = expr_addr_to_obj_addr(vector_addr_expr);

  const vector_config_t &cfg = ctx.get_vector_config(obj);

  const vector_table_data_t data = {
      .obj      = obj,
      .capacity = static_cast<u32>(cfg.capacity),
      .key      = index,
      .value    = cell,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> VectorTableLookupFactory::speculate(const EP *ep, const BDDNode *node, const Context &ctx) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (!call_node->is_vector_read()) {
    return {};
  }

  const vector_table_data_t data = get_vector_table_data(ep->get_ctx(), call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  if (!can_build_or_reuse_vector_table(ep, node, data)) {
    return {};
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(data.obj, DSImpl::Tofino_VectorTable);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> VectorTableLookupFactory::process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (!call_node->is_vector_read()) {
    return {};
  }

  const vector_table_data_t data = get_vector_table_data(ep->get_ctx(), call_node);

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  VectorTable *vector_table = build_or_reuse_vector_table(ep, node, data);

  if (!vector_table) {
    return {};
  }

  Module *module  = new VectorTableLookup(type, node, vector_table->id, data.obj, data.key, data.value);
  EPNode *ep_node = new EPNode(module);

  std::unique_ptr<EP> new_ep = std::make_unique<EP>(*ep);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(data.obj, DSImpl::Tofino_VectorTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep.get());
  tofino_ctx->place(new_ep.get(), node, data.obj, vector_table);

  const EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  std::vector<impl_t> impls;
  impls.emplace_back(implement(ep, node, std::move(new_ep)));
  return impls;
}

std::unique_ptr<Module> VectorTableLookupFactory::create(const BDD *bdd, const Context &ctx, const BDDNode *node) const {
  if (node->get_type() != BDDNodeType::Call) {
    return {};
  }

  const Call *call_node = dynamic_cast<const Call *>(node);
  const call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (!call_node->is_vector_read()) {
    return {};
  }

  const vector_table_data_t data = get_vector_table_data(ctx, call_node);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  const std::unordered_set<Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(data.obj);

  const VectorTable *vector_table = dynamic_cast<const VectorTable *>(*ds.begin());

  return std::make_unique<VectorTableLookup>(type, node, vector_table->id, data.obj, data.key, data.value);
}

} // namespace Tofino
} // namespace LibSynapse