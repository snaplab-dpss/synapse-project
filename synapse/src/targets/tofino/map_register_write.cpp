#include "map_register_write.h"

namespace synapse {
namespace tofino {
namespace {
struct map_register_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;
  u32 num_entries;

  map_register_data_t(const EP *ep, std::vector<const Call *> future_map_puts) {
    SYNAPSE_ASSERT(!future_map_puts.empty(), "No future map puts");
    const Call *map_put = future_map_puts.front();
    const call_t &put_call = map_put->get_call();

    obj = expr_addr_to_obj_addr(put_call.args.at("map").expr);
    key = put_call.args.at("key").in;
    write_value = put_call.args.at("value").expr;
    num_entries = ep->get_ctx().get_map_config(obj).capacity;
  }
};

std::vector<const Node *> get_nodes_to_speculatively_ignore(const EP *ep,
                                                            const Call *dchain_allocate_new_index,
                                                            const map_coalescing_objs_t &map_objs,
                                                            klee::ref<klee::Expr> key) {
  std::vector<const Node *> nodes_to_ignore;

  const BDD *bdd = ep->get_bdd();
  std::vector<const Call *> coalescing_nodes =
      get_coalescing_nodes_from_key(bdd, dchain_allocate_new_index, key, map_objs);

  for (const Call *coalescing_node : coalescing_nodes) {
    nodes_to_ignore.push_back(coalescing_node);
  }

  branch_direction_t index_alloc_check =
      find_branch_checking_index_alloc(ep, dchain_allocate_new_index);

  if (index_alloc_check.branch) {
    nodes_to_ignore.push_back(index_alloc_check.branch);

    const Node *next = index_alloc_check.direction ? index_alloc_check.branch->get_on_false()
                                                   : index_alloc_check.branch->get_on_true();

    next->visit_nodes([&nodes_to_ignore](const Node *node) {
      nodes_to_ignore.push_back(node);
      return NodeVisitAction::Continue;
    });
  }

  return nodes_to_ignore;
}

std::unique_ptr<BDD>
delete_coalescing_nodes(const EP *ep, const Node *node, const map_coalescing_objs_t &map_objs,
                        klee::ref<klee::Expr> key, const Node *&new_next,
                        std::optional<constraints_t> &deleted_branch_constraints) {
  const BDD *old_bdd = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const Node *next = node->get_next();

  if (next) {
    new_next = new_bdd->get_node_by_id(next->get_id());
  } else {
    new_next = nullptr;
  }

  const std::vector<const Call *> targets =
      get_coalescing_nodes_from_key(new_bdd.get(), new_next, key, map_objs);

  for (const Node *target : targets) {
    const Call *call_target = dynamic_cast<const Call *>(target);
    const call_t &call = call_target->get_call();

    if (call.function_name == "dchain_allocate_new_index") {
      symbol_t out_of_space = call_target->get_local_symbol("out_of_space");
      branch_direction_t index_alloc_check =
          find_branch_checking_index_alloc(ep, node, out_of_space);

      if (index_alloc_check.branch) {
        SYNAPSE_ASSERT(!deleted_branch_constraints.has_value(),
                       "Multiple branch checking index allocation detected");
        deleted_branch_constraints = index_alloc_check.branch->get_ordered_branch_constraints();

        klee::ref<klee::Expr> extra_constraint = index_alloc_check.branch->get_condition();

        // If we want to keep the direction on true, we must remove the on
        // false.
        if (index_alloc_check.direction) {
          extra_constraint = solver_toolbox.exprBuilder->Not(extra_constraint);
        }

        deleted_branch_constraints->push_back(extra_constraint);

        bool replace_next = (index_alloc_check.branch == next);
        Node *replacement = delete_branch_node_from_bdd(
            new_bdd.get(), index_alloc_check.branch->get_id(), index_alloc_check.direction);
        if (replace_next) {
          new_next = replacement;
        }
      }
    }

    bool replace_next = (target == next);
    Node *replacement = delete_non_branch_node_from_bdd(new_bdd.get(), target->get_id());
    if (replace_next) {
      new_next = replacement;
    }
  }

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> MapRegisterWriteFactory::speculate(const EP *ep, const Node *node,
                                                              const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!is_map_update_with_dchain(ep, dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return std::nullopt;
  }

  SYNAPSE_ASSERT(!future_map_puts.empty(), "No future map puts");

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index, map_objs)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
      !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister)) {
    return std::nullopt;
  }

  map_register_data_t map_register_data(ep, future_map_puts);

  if (!can_build_or_reuse_map_register(ep, node, map_register_data.obj, map_register_data.key,
                                       map_register_data.num_entries)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
  new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  std::vector<const Node *> ignore_nodes = get_nodes_to_speculatively_ignore(
      ep, dchain_allocate_new_index, map_objs, map_register_data.key);

  for (const Node *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t> MapRegisterWriteFactory::process_node(const EP *ep, const Node *node,
                                                          SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *dchain_allocate_new_index = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!is_map_update_with_dchain(ep, dchain_allocate_new_index, future_map_puts)) {
    // The cached table read should deal with these cases.
    return impls;
  }

  SYNAPSE_ASSERT(!future_map_puts.empty(), "No future map puts");

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index, map_objs)) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
      !ep->get_ctx().can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister)) {
    return impls;
  }

  map_register_data_t map_register_data(ep, future_map_puts);

  MapRegister *map_register = build_or_reuse_map_register(
      ep, node, map_register_data.obj, map_register_data.key, map_register_data.num_entries);

  if (!map_register) {
    return impls;
  }

  Module *module = new MapRegisterWrite(node, map_register->id, map_register_data.obj,
                                        map_register_data.key, map_register_data.write_value);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  const Node *new_next;
  std::optional<constraints_t> deleted_branch_constraints;
  std::unique_ptr<BDD> new_bdd = delete_coalescing_nodes(
      new_ep, node, map_objs, map_register_data.key, new_next, deleted_branch_constraints);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

  if (deleted_branch_constraints.has_value()) {
    ctx.get_mutable_profiler().remove(deleted_branch_constraints.value());
  }

  TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
  tofino_ctx->place(new_ep, node, map_objs.map, map_register);

  EPLeaf leaf(ep_node, new_next);
  new_ep->process_leaf(ep_node, {leaf});
  new_ep->replace_bdd(std::move(new_bdd));
  new_ep->assert_integrity();

  return impls;
}

} // namespace tofino
} // namespace synapse