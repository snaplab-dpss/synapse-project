#include "map_register_read_or_write.h"

#include "if.h"
#include "then.h"
#include "else.h"
#include "send_to_controller.h"

namespace synapse {
namespace tofino {
namespace {
struct map_register_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;
  u32 num_entries;

  map_register_data_t(const EP *ep, const Call *map_get, const Call *map_put) {
    const call_t &get_call = map_get->get_call();
    const call_t &put_call = map_put->get_call();

    obj = expr_addr_to_obj_addr(get_call.args.at("map").expr);
    key = get_call.args.at("key").in;
    read_value = get_call.args.at("value_out").out;
    write_value = put_call.args.at("value").expr;
    map_has_this_key = map_get->get_local_symbol("map_has_this_key");
    num_entries = ep->get_ctx().get_map_config(obj).capacity;
  }
};

std::vector<const Node *> get_nodes_to_speculatively_ignore(const EP *ep, const Node *on_success,
                                                            const map_coalescing_objs_t &map_objs,
                                                            klee::ref<klee::Expr> key) {
  std::vector<const Node *> nodes_to_ignore;

  const BDD *bdd = ep->get_bdd();
  std::vector<const Call *> coalescing_nodes =
      get_coalescing_nodes_from_key(bdd, on_success, key, map_objs);

  for (const Call *coalescing_node : coalescing_nodes) {
    nodes_to_ignore.push_back(coalescing_node);

    const call_t &call = coalescing_node->get_call();

    if (call.function_name == "dchain_allocate_new_index") {
      symbol_t out_of_space = coalescing_node->get_local_symbol("out_of_space");
      branch_direction_t index_alloc_check =
          find_branch_checking_index_alloc(ep, on_success, out_of_space);

      // FIXME: We ignore all logic happening when the index is not
      // successfuly allocated. This is a major simplification, as the NF
      // might be doing something useful here. We never encountered a scenario
      // where this was the case, but it could happen nonetheless.
      if (index_alloc_check.branch) {
        nodes_to_ignore.push_back(index_alloc_check.branch);

        const Node *next = index_alloc_check.direction ? index_alloc_check.branch->get_on_false()
                                                       : index_alloc_check.branch->get_on_true();

        next->visit_nodes([&nodes_to_ignore](const Node *node) {
          nodes_to_ignore.push_back(node);
          return NodeVisitAction::Continue;
        });
      }
    }
  }

  return nodes_to_ignore;
}

void update_profiler(const BDD *bdd, Profiler &profiler, const map_rw_pattern_t &map_rw_pattern) {
  hit_rate_t on_index_alloc_success_hr =
      profiler.get_hr(map_rw_pattern.index_alloc_check.direction
                          ? map_rw_pattern.index_alloc_check.branch->get_on_true()
                          : map_rw_pattern.index_alloc_check.branch->get_on_false());

  hit_rate_t on_index_alloc_failed_hr =
      profiler.get_hr(map_rw_pattern.index_alloc_check.direction
                          ? map_rw_pattern.index_alloc_check.branch->get_on_false()
                          : map_rw_pattern.index_alloc_check.branch->get_on_true());

  const Branch *index_alloc_check = dynamic_cast<const Branch *>(
      bdd->get_node_by_id(map_rw_pattern.index_alloc_check.branch->get_id()));

  profiler.replace_constraint(
      map_rw_pattern.index_alloc_check.branch->get_ordered_branch_constraints(),
      index_alloc_check->get_condition());

  if (map_rw_pattern.map_put_extra_condition.branch) {
    const Node *to_be_removed = map_rw_pattern.map_put_extra_condition.direction
                                    ? map_rw_pattern.map_put_extra_condition.branch->get_on_false()
                                    : map_rw_pattern.map_put_extra_condition.branch->get_on_true();

    hit_rate_t removed_hr = profiler.get_hr(to_be_removed);

    profiler.remove(to_be_removed->get_ordered_branch_constraints());

    hit_rate_t new_on_index_alloc_success_hr = on_index_alloc_success_hr;
    hit_rate_t new_on_index_alloc_failed_hr = on_index_alloc_failed_hr + removed_hr;

    const Node *on_index_alloc_success = map_rw_pattern.index_alloc_check.direction
                                             ? index_alloc_check->get_on_true()
                                             : index_alloc_check->get_on_false();
    const Node *on_index_alloc_failed = map_rw_pattern.index_alloc_check.direction
                                            ? index_alloc_check->get_on_false()
                                            : index_alloc_check->get_on_true();

    profiler.set(on_index_alloc_success->get_ordered_branch_constraints(),
                 new_on_index_alloc_success_hr);
    profiler.set(on_index_alloc_failed->get_ordered_branch_constraints(),
                 new_on_index_alloc_failed_hr);
  }
}

std::unique_ptr<BDD> rebuild_bdd(const EP *ep, const Node *node,
                                 const map_rw_pattern_t &map_rw_pattern,
                                 const symbol_t &map_reg_successful_write, const Node *&new_next) {
  const BDD *old_bdd = ep->get_bdd();
  std::unique_ptr<BDD> new_bdd = std::make_unique<BDD>(*old_bdd);

  const Node *next = node->get_next();
  new_next = new_bdd->get_node_by_id(next->get_id());

  delete_non_branch_node_from_bdd(new_bdd.get(),
                                  map_rw_pattern.dchain_allocate_new_index->get_id());

  if (map_rw_pattern.map_put_extra_condition.branch) {
    delete_branch_node_from_bdd(new_bdd.get(),
                                map_rw_pattern.map_put_extra_condition.branch->get_id(),
                                map_rw_pattern.map_put_extra_condition.direction);
  }

  Branch *index_alloc_check_node = dynamic_cast<Branch *>(
      new_bdd->get_mutable_node_by_id(map_rw_pattern.index_alloc_check.branch->get_id()));

  if (map_rw_pattern.index_alloc_check.direction) {
    index_alloc_check_node->set_condition(solver_toolbox.exprBuilder->Ne(
        map_reg_successful_write.expr,
        solver_toolbox.exprBuilder->Constant(0, map_reg_successful_write.expr->getWidth())));
  } else {
    index_alloc_check_node->set_condition(solver_toolbox.exprBuilder->Eq(
        map_reg_successful_write.expr,
        solver_toolbox.exprBuilder->Constant(0, map_reg_successful_write.expr->getWidth())));
  }

  delete_non_branch_node_from_bdd(new_bdd.get(), map_rw_pattern.map_put->get_id());

  return new_bdd;
}
} // namespace

std::optional<spec_impl_t> MapRegisterReadOrWriteFactory::speculate(const EP *ep, const Node *node,
                                                                    const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!is_map_get_followed_by_map_puts_on_miss(ep->get_bdd(), map_get, future_map_puts)) {
    // The cached table read should deal with these cases.
    return std::nullopt;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
    return std::nullopt;
  }

  if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
      !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister)) {
    return std::nullopt;
  }

  map_register_data_t map_register_data(ep, map_get, future_map_puts[0]);

  if (!can_build_or_reuse_map_register(ep, node, map_register_data.obj, map_register_data.key,
                                       map_register_data.num_entries)) {
    return std::nullopt;
  }

  Context new_ctx = ctx;
  new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
  new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  // FIXME: we are ignoring more than we should!
  std::vector<const Node *> ignore_nodes =
      get_nodes_to_speculatively_ignore(ep, map_get, map_objs, map_register_data.key);

  for (const Node *op : ignore_nodes) {
    spec_impl.skip.insert(op->get_id());
  }

  return spec_impl;
}

std::vector<impl_t>
MapRegisterReadOrWriteFactory::process_node(const EP *ep, const Node *node,
                                            SymbolManager *symbol_manager) const {
  std::vector<impl_t> impls;

  if (node->get_type() != NodeType::Call) {
    return impls;
  }

  const Call *map_get = dynamic_cast<const Call *>(node);

  map_rw_pattern_t map_rw_pattern;
  if (!is_compact_map_get_followed_by_map_put_on_miss(ep, map_get, map_rw_pattern)) {
    return impls;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
    return impls;
  }

  if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
      !ep->get_ctx().can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister)) {
    return impls;
  }

  map_register_data_t map_register_data(ep, map_get, map_rw_pattern.map_put);

  MapRegister *map_register = build_or_reuse_map_register(
      ep, node, map_register_data.obj, map_register_data.key, map_register_data.num_entries);

  if (!map_register) {
    return impls;
  }

  // FIXME:
  // symbol_t map_reg_successful_write = create_symbol("map_reg_successful_write", 32);
  symbol_t map_reg_successful_write;

  Module *module = new MapRegisterReadOrWrite(
      node, map_register->id, map_register_data.obj, map_register_data.key,
      map_register_data.read_value, map_register_data.write_value,
      map_rw_pattern.map_put_extra_condition.branch
          ? map_rw_pattern.map_put_extra_condition.branch->get_condition()
          : nullptr,
      map_register_data.map_has_this_key, map_reg_successful_write);
  EPNode *ep_node = new EPNode(module);

  EP *new_ep = new EP(*ep);
  impls.push_back(implement(ep, node, new_ep));

  const Node *new_next;
  std::unique_ptr<BDD> new_bdd =
      rebuild_bdd(new_ep, node, map_rw_pattern, map_reg_successful_write, new_next);

  Context &ctx = new_ep->get_mutable_ctx();
  ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
  ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

  update_profiler(new_bdd.get(), ctx.get_mutable_profiler(), map_rw_pattern);

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