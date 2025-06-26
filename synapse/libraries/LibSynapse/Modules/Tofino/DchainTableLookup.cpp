#include <LibSynapse/Modules/Tofino/DchainTableLookup.h>
#include <LibSynapse/ExecutionPlan.h>

namespace LibSynapse {
namespace Tofino {

namespace {

dchain_table_data_t get_dchain_table_data(const Context &ctx, const LibBDD::Call *call_node) {
  const LibBDD::call_t &call = call_node->get_call();
  assert((call.function_name == "dchain_is_index_allocated" || call.function_name == "dchain_rejuvenate_index") && "Unexpected function");

  klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
  klee::ref<klee::Expr> index            = call.args.at("index").expr;

  addr_t dchain_addr = LibCore::expr_addr_to_obj_addr(dchain_addr_expr);

  const LibBDD::dchain_config_t &cfg = ctx.get_dchain_config(dchain_addr);

  dchain_table_data_t data = {
      .obj      = dchain_addr,
      .capacity = static_cast<u32>(cfg.index_range),
      .key      = index,
      .hit      = std::nullopt,
  };

  if (call.function_name == "dchain_is_index_allocated") {
    data.hit = call_node->get_local_symbol("is_index_allocated");
  }

  return data;
}

} // namespace

std::optional<spec_impl_t> DchainTableLookupFactory::speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return std::nullopt;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated" && call.function_name != "dchain_rejuvenate_index") {
    return std::nullopt;
  }

  dchain_table_data_t data = get_dchain_table_data(ep->get_ctx(), call_node);

  if (!ctx.can_impl_ds(data.obj, DSImpl::Tofino_DchainTable)) {
    return std::nullopt;
  }

  if (!can_build_or_reuse_dchain_table(ep, node, data)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(data.obj, DSImpl::Tofino_DchainTable);

  return spec_impl_t(decide(ep, node), new_ctx);
}

std::vector<impl_t> DchainTableLookupFactory::process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != LibBDD::NodeType::Call) {
    return impls;
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated" && call.function_name != "dchain_rejuvenate_index") {
    return impls;
  }

  dchain_table_data_t data = get_dchain_table_data(ep->get_ctx(), call_node);

  if (!ep->get_ctx().can_impl_ds(data.obj, DSImpl::Tofino_DchainTable)) {
    return impls;
  }

  DchainTable *dchain_table = build_or_reuse_dchain_table(ep, node, data);

  if (!dchain_table) {
    return impls;
  }

  Module *module  = new DchainTableLookup(node, dchain_table->id, data.obj, data.key, data.hit);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(data.obj, DSImpl::Tofino_DchainTable);

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, data.obj, dchain_table);

  EPLeaf leaf(ep_node, node->get_next());
  new_ep->process_leaf(ep_node, {leaf});

  return impls;
}

std::unique_ptr<Module> DchainTableLookupFactory::create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const {
  if (node->get_type() != LibBDD::NodeType::Call) {
    return {};
  }

  const LibBDD::Call *call_node = dynamic_cast<const LibBDD::Call *>(node);
  const LibBDD::call_t &call    = call_node->get_call();

  if (call.function_name != "dchain_is_index_allocated" && call.function_name != "dchain_rejuvenate_index") {
    return {};
  }

  dchain_table_data_t data = get_dchain_table_data(ctx, call_node);

  if (!ctx.check_ds_impl(data.obj, DSImpl::Tofino_DchainTable)) {
    return {};
  }

  const std::unordered_set<LibSynapse::Tofino::DS *> ds = ctx.get_target_ctx<TofinoContext>()->get_data_structures().get_ds(data.obj);

  const DchainTable *dchain_table = dynamic_cast<const DchainTable *>(*ds.begin());

  return std::make_unique<DchainTableLookup>(node, dchain_table->id, data.obj, data.key, data.hit);
}

} // namespace Tofino
} // namespace LibSynapse