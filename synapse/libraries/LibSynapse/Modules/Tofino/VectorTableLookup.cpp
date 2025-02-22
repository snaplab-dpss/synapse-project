#include <LibSynapse/Modules/Tofino/VectorTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {

vector_table_data_t get_vector_table_data(const Context &ctx, const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert(call.function_name == "vector_borrow" && "Unexpected function");

  klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;
  klee::ref<klee::Expr> cell             = call.extra_vars.at("borrowed_cell").second;

  addr_t obj = LibCore::expr_addr_to_obj_addr(vector_addr_expr);

  const LibBDD::vector_config_t &cfg = ctx.get_vector_config(obj);

  vector_table_data_t data = {
      .obj         = obj,
      .num_entries = static_cast<u32>(cfg.capacity),
      .key         = index,
      .value       = cell,
  };

  return data;
}

} // namespace

std::optional<spec_impl_t> VectorTableLookupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return std::nullopt;
  }

  if (!call_node->is_vector_read()) {
    return std::nullopt;
  }

  vector_table_data_t data = get_vector_table_data(ep->get_ctx(), call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_VectorTable)) {
    return std::nullopt;
  }

  if (!can_build_or_reuse_vector_table(ep, node, data)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(data.obj, DSImpl::Tofino_VectorTable);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> VectorTableLookupFactory::process_node(const EP *ep, const LibBDD::Node *node,
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

  if (!call_node->is_vector_read()) {
    return impls;
  }

  vector_table_data_t data = get_vector_table_data(ep->get_ctx(), call_node);

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Tofino_VectorTable)) {
    return impls;
  }

  VectorTable *vector_table = build_or_reuse_vector_table(ep, node, data);

  if (!vector_table) {
    return impls;
  }

  Module *module  = new VectorTableLookup(node, vector_table->id, data.obj, data.key, data.value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(data.obj, DSImpl::Tofino_VectorTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, data.obj, vector_table);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> VectorTableLookupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "vector_borrow") {
    return {};
  }

  if (!call_node->is_vector_read()) {
    return {};
  }

  vector_table_data_t data = get_vector_table_data(ctx, call_node);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_VectorTable)) {
    return {};
  }

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_ds(data.obj);

  const VectorTable *vector_table = dynamic_cast<const VectorTable *>(*ds.begin());

  return std::make_unique<VectorTableLookup>(node, vector_table->id, data.obj, data.key, data.value);
}

} // namespace Tofino
} // namespace LibSynapse