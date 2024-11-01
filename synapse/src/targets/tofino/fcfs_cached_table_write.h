#pragma once

#include "tofino_module.h"

#include "if.h"
#include "then.h"
#include "else.h"
#include "send_to_controller.h"

namespace tofino {

class FCFSCachedTableWrite : public TofinoModule {
private:
  DS_ID cached_table_id;
  std::unordered_set<DS_ID> cached_table_bydecisions;

  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;
  symbol_t cache_write_failed;

public:
  FCFSCachedTableWrite(
      const Node *node, DS_ID _cached_table_id,
      const std::unordered_set<DS_ID> &_cached_table_bydecisions, addr_t _obj,
      klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _write_value,
      const symbol_t &_cache_write_failed)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableWrite,
                     "FCFSCachedTableWrite", node),
        cached_table_id(_cached_table_id),
        cached_table_bydecisions(_cached_table_bydecisions), obj(_obj),
        key(_key), write_value(_write_value),
        cache_write_failed(_cache_write_failed) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableWrite(
        node, cached_table_id, cached_table_bydecisions, obj, key, write_value,
        cache_write_failed);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const symbol_t &get_cache_write_failed() const { return cache_write_failed; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return cached_table_bydecisions;
  }
};

class FCFSCachedTableWriteGenerator : public TofinoModuleGenerator {
public:
  FCFSCachedTableWriteGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_FCFSCachedTableWrite,
                              "FCFSCachedTableWrite") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *dchain_allocate_new_index = static_cast<const Call *>(node);

    std::vector<const Call *> future_map_puts;
    if (!is_map_update_with_dchain(ep, dchain_allocate_new_index,
                                   future_map_puts)) {
      // The cached table read should deal with these cases.
      return std::nullopt;
    }

    assert(!future_map_puts.empty());

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index,
                                                map_objs)) {
      return std::nullopt;
    }

    if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
        !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable) ||
        !ctx.can_impl_ds(map_objs.vector_key, DSImpl::Tofino_FCFSCachedTable)) {
      return std::nullopt;
    }

    fcfs_cached_table_data_t cached_table_data =
        get_cached_table_data(ep, future_map_puts);

    std::vector<int> allowed_cache_capacities =
        enum_fcfs_cache_cap(cached_table_data.num_entries);

    hit_rate_t chosen_success_estimation = 0;
    int chosen_cache_capacity = 0;
    bool successfully_placed = false;

    // We can use a different method for picking the right estimation depending
    // on the time it takes to find a solution.
    for (int cache_capacity : allowed_cache_capacities) {
      hit_rate_t success_estimation = get_cache_success_estimation_rel(
          ep, node, future_map_puts[0], cached_table_data.key, cache_capacity);

      if (!can_get_or_build_fcfs_cached_table(
              ep, node, cached_table_data.obj, cached_table_data.key,
              cached_table_data.num_entries, cache_capacity)) {
        break;
      }

      if (success_estimation > chosen_success_estimation) {
        chosen_success_estimation = success_estimation;
        chosen_cache_capacity = cache_capacity;
      }

      successfully_placed = true;
    }

    if (!successfully_placed) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    const Profiler &profiler = new_ctx.get_profiler();

    hit_rate_t fraction = profiler.get_hr(node);
    hit_rate_t on_fail_fraction = fraction * (1 - chosen_success_estimation);

    // FIXME: not using profiler cache.
    constraints_t constraints = node->get_ordered_branch_constraints();

    new_ctx.scale_profiler(constraints, chosen_success_estimation);
    new_ctx.save_ds_impl(cached_table_data.obj, DSImpl::Tofino_FCFSCachedTable);

    auto perf_estimator_fn = [chosen_success_estimation](const Context &ctx,
                                                         const Node *node,
                                                         pps_t ingress) {
      return chosen_success_estimation * ingress;
    };

    auto perf_sink_fn = [on_fail_fraction](const Context &ctx, const Node *node,
                                           pps_t ingress) {
      return on_fail_fraction * ingress;
    };

    spec_impl_t spec_impl(
        decide(ep, node, {{CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx,
        perf_estimator_fn, perf_sink_fn);

    std::vector<const Node *> ignore_nodes = get_nodes_to_speculatively_ignore(
        ep, dchain_allocate_new_index, map_objs, cached_table_data.key);

    for (const Node *op : ignore_nodes) {
      spec_impl.skip.insert(op->get_id());
    }

    return spec_impl;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *dchain_allocate_new_index = static_cast<const Call *>(node);

    std::vector<const Call *> future_map_puts;
    if (!is_map_update_with_dchain(ep, dchain_allocate_new_index,
                                   future_map_puts)) {
      // The cached table read should deal with these cases.
      return impls;
    }

    assert(!future_map_puts.empty());

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index,
                                                map_objs)) {
      return impls;
    }

    if (!ep->get_ctx().can_impl_ds(map_objs.map,
                                   DSImpl::Tofino_FCFSCachedTable) ||
        !ep->get_ctx().can_impl_ds(map_objs.dchain,
                                   DSImpl::Tofino_FCFSCachedTable) ||
        !ep->get_ctx().can_impl_ds(map_objs.vector_key,
                                   DSImpl::Tofino_FCFSCachedTable)) {
      return impls;
    }

    symbol_t cache_write_failed = create_symbol("cache_write_failed", 32);

    fcfs_cached_table_data_t cached_table_data =
        get_cached_table_data(ep, future_map_puts);

    std::vector<int> allowed_cache_capacities =
        enum_fcfs_cache_cap(cached_table_data.num_entries);

    for (int cache_capacity : allowed_cache_capacities) {
      std::optional<impl_t> impl = concretize_cached_table_write(
          ep, node, map_objs, cached_table_data, cache_write_failed,
          cache_capacity, future_map_puts);

      if (impl.has_value()) {
        impls.push_back(*impl);
      } else {
        // No need to try bigger caches if we can't even fit a smaller one.
        break;
      }
    }

    return impls;
  }

private:
  struct fcfs_cached_table_data_t {
    addr_t obj;
    klee::ref<klee::Expr> key;
    klee::ref<klee::Expr> read_value;
    klee::ref<klee::Expr> write_value;
    symbol_t map_has_this_key;
    int num_entries;
  };

  std::optional<impl_t> concretize_cached_table_write(
      const EP *ep, const Node *node, const map_coalescing_objs_t &map_objs,
      const fcfs_cached_table_data_t &cached_table_data,
      const symbol_t &cache_write_failed, int cache_capacity,
      const std::vector<const Call *> &future_map_puts) const {
    FCFSCachedTable *cached_table = build_or_reuse_fcfs_cached_table(
        ep, node, cached_table_data.obj, cached_table_data.key,
        cached_table_data.num_entries, cache_capacity);

    if (!cached_table) {
      return std::nullopt;
    }

    std::unordered_set<DS_ID> bydecisions =
        get_cached_table_bydecisions(cached_table);

    klee::ref<klee::Expr> cache_write_success_condition =
        build_cache_write_success_condition(cache_write_failed);

    Module *module = new FCFSCachedTableWrite(
        node, cached_table->id, bydecisions, cached_table_data.obj,
        cached_table_data.key, cached_table_data.write_value,
        cache_write_failed);
    EPNode *cached_table_write_node = new EPNode(module);

    EP *new_ep = new EP(*ep);

    Node *on_cache_write_success;
    Node *on_cache_write_failed;
    std::optional<constraints_t> deleted_branch_constraints;

    BDD *new_bdd = branch_bdd_on_cache_write_success(
        new_ep, node, cached_table_data, cache_write_success_condition,
        map_objs, on_cache_write_success, on_cache_write_failed,
        deleted_branch_constraints);

    symbols_t symbols = get_dataplane_state(ep, node);

    Module *if_module = new If(node, cache_write_success_condition,
                               {cache_write_success_condition});
    Module *then_module = new Then(node);
    Module *else_module = new Else(node);
    Module *send_to_controller_module =
        new SendToController(on_cache_write_failed, symbols);

    EPNode *if_node = new EPNode(if_module);
    EPNode *then_node = new EPNode(then_module);
    EPNode *else_node = new EPNode(else_module);
    EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

    cached_table_write_node->set_children({if_node});
    if_node->set_prev(cached_table_write_node);

    if_node->set_children({then_node, else_node});
    then_node->set_prev(if_node);
    else_node->set_prev(if_node);

    else_node->set_children({send_to_controller_node});
    send_to_controller_node->set_prev(else_node);

    hit_rate_t cache_write_success_estimation_rel =
        get_cache_success_estimation_rel(ep, node, future_map_puts[0],
                                         cached_table_data.key, cache_capacity);

    new_ep->update_node_constraints(then_node, else_node,
                                    cache_write_success_condition);

    new_ep->add_hit_rate_estimation(cache_write_success_condition,
                                    cache_write_success_estimation_rel);

    if (deleted_branch_constraints.has_value()) {
      new_ep->remove_hit_rate_node(deleted_branch_constraints.value());
    }

    place_fcfs_cached_table(new_ep, node, map_objs, cached_table);

    EPLeaf on_cache_write_success_leaf(then_node, on_cache_write_success);
    EPLeaf on_cache_write_failed_leaf(send_to_controller_node,
                                      on_cache_write_failed);

    new_ep->process_leaf(cached_table_write_node, {on_cache_write_success_leaf,
                                                   on_cache_write_failed_leaf});
    new_ep->replace_bdd(new_bdd);
    // new_ep->inspect();

    return implement(ep, node, new_ep, {{CACHE_SIZE_PARAM, cache_capacity}});
  }

  hit_rate_t get_cache_success_estimation_rel(const EP *ep, const Node *node,
                                              const Node *map_put,
                                              klee::ref<klee::Expr> key,
                                              int cache_capacity) const {
    const Context &ctx = ep->get_ctx();
    const Profiler &profiler = ctx.get_profiler();

    hit_rate_t fraction = profiler.get_hr(node);

    // FIXME: not using profiler cache.
    constraints_t full_write_constraints =
        map_put->get_ordered_branch_constraints();
    std::optional<FlowStats> flow_stats =
        profiler.get_flow_stats(full_write_constraints, key);
    assert(flow_stats.has_value());

    u64 cached_packets = std::min(
        flow_stats->packets, flow_stats->avg_pkts_per_flow * cache_capacity);
    hit_rate_t expected_cached_fraction =
        cached_packets / static_cast<hit_rate_t>(flow_stats->packets);

    return fraction * expected_cached_fraction;
  }

  std::unordered_set<DS_ID>
  get_cached_table_bydecisions(FCFSCachedTable *cached_table) const {
    std::unordered_set<DS_ID> bydecisions;

    std::vector<std::unordered_set<const DS *>> internal_ds =
        cached_table->get_internal_ds();
    for (const std::unordered_set<const DS *> &ds_set : internal_ds) {
      for (const DS *ds : ds_set) {
        bydecisions.insert(ds->id);
      }
    }

    return bydecisions;
  }

  fcfs_cached_table_data_t
  get_cached_table_data(const EP *ep,
                        std::vector<const Call *> future_map_puts) const {
    fcfs_cached_table_data_t cached_table_data;

    assert(!future_map_puts.empty());
    const Call *map_put = future_map_puts.front();

    const call_t &put_call = map_put->get_call();
    klee::ref<klee::Expr> obj_expr = put_call.args.at("map").expr;

    cached_table_data.obj = expr_addr_to_obj_addr(obj_expr);
    cached_table_data.key = put_call.args.at("key").in;
    cached_table_data.write_value = put_call.args.at("value").expr;

    const Context &ctx = ep->get_ctx();
    const map_config_t &cfg = ctx.get_map_config(cached_table_data.obj);

    cached_table_data.num_entries = cfg.capacity;
    return cached_table_data;
  }

  std::vector<const Node *> get_nodes_to_speculatively_ignore(
      const EP *ep, const Call *dchain_allocate_new_index,
      const map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key) const {
    std::vector<const Node *> nodes_to_ignore;

    const BDD *bdd = ep->get_bdd();
    std::vector<const Call *> coalescing_nodes = get_coalescing_nodes_from_key(
        bdd, dchain_allocate_new_index, key, map_objs);

    for (const Call *coalescing_node : coalescing_nodes) {
      nodes_to_ignore.push_back(coalescing_node);
    }

    symbol_t out_of_space;
    bool found =
        get_symbol(dchain_allocate_new_index->get_locally_generated_symbols(),
                   "out_of_space", out_of_space);
    assert(found && "Symbol out_of_space not found");

    const Branch *branch = find_branch_checking_index_alloc(
        ep, dchain_allocate_new_index, out_of_space);

    if (branch) {
      nodes_to_ignore.push_back(branch);

      bool direction_to_keep = true;
      const Node *next =
          direction_to_keep ? branch->get_on_false() : branch->get_on_true();

      next->visit_nodes([&nodes_to_ignore](const Node *node) {
        nodes_to_ignore.push_back(node);
        return NodeVisitAction::VISIT_CHILDREN;
      });
    }

    return nodes_to_ignore;
  }

  klee::ref<klee::Expr> build_cache_write_success_condition(
      const symbol_t &cache_write_failed) const {
    klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(
        0, cache_write_failed.expr->getWidth());
    return solver_toolbox.exprBuilder->Eq(cache_write_failed.expr, zero);
  }

  void add_dchain_allocate_new_index_clone_on_cache_write_failed(
      const EP *ep, BDD *bdd, const Node *dchain_allocate_new_index,
      const Branch *cache_write_branch,
      Node *&new_on_cache_write_failed) const {
    node_id_t &id = bdd->get_mutable_id();
    NodeManager &manager = bdd->get_mutable_manager();
    Node *dchain_allocate_new_index_on_cache_write_failed =
        dchain_allocate_new_index->clone(manager, false);
    dchain_allocate_new_index_on_cache_write_failed->recursive_update_ids(id);

    add_non_branch_nodes_to_bdd(
        ep, bdd, cache_write_branch->get_on_false(),
        {dchain_allocate_new_index_on_cache_write_failed},
        new_on_cache_write_failed);
  }

  void replicate_hdr_parsing_ops_on_cache_write_failed(
      const EP *ep, BDD *bdd, const Branch *cache_write_branch,
      Node *&new_on_cache_write_failed) const {
    const Node *on_cache_write_failed = cache_write_branch->get_on_false();

    std::vector<const Call *> prev_borrows = get_prev_functions(
        ep, on_cache_write_failed, {"packet_borrow_next_chunk"});

    if (prev_borrows.empty()) {
      return;
    }

    std::vector<const Node *> non_branch_nodes_to_add;
    for (const Call *prev_borrow : prev_borrows) {
      non_branch_nodes_to_add.push_back(prev_borrow);
    }

    add_non_branch_nodes_to_bdd(ep, bdd, on_cache_write_failed,
                                non_branch_nodes_to_add,
                                new_on_cache_write_failed);
  }

  void delete_coalescing_nodes_on_success(
      const EP *ep, BDD *bdd, Node *on_success,
      const map_coalescing_objs_t &map_objs, klee::ref<klee::Expr> key,
      std::optional<constraints_t> &deleted_branch_constraints) const {
    const std::vector<const Call *> targets =
        get_coalescing_nodes_from_key(bdd, on_success, key, map_objs);

    for (const Node *target : targets) {
      const Call *call_target = static_cast<const Call *>(target);
      const call_t &call = call_target->get_call();

      if (call.function_name == "dchain_allocate_new_index") {
        symbol_t out_of_space;
        bool found = get_symbol(call_target->get_locally_generated_symbols(),
                                "out_of_space", out_of_space);
        assert(found && "Symbol out_of_space not found");

        const Branch *branch =
            find_branch_checking_index_alloc(ep, on_success, out_of_space);

        if (branch) {
          assert(!deleted_branch_constraints.has_value() &&
                 "Multiple branch checking index allocation detected");
          deleted_branch_constraints = branch->get_ordered_branch_constraints();
          bool direction_to_keep = true;

          klee::ref<klee::Expr> extra_constraint = branch->get_condition();

          // If we want to keep the direction on true, we must remove the on
          // false.
          if (direction_to_keep) {
            extra_constraint =
                solver_toolbox.exprBuilder->Not(extra_constraint);
          }

          deleted_branch_constraints->push_back(extra_constraint);

          Node *trash;
          delete_branch_node_from_bdd(ep, bdd, branch->get_id(),
                                      direction_to_keep, trash);
        }
      }

      Node *trash;
      delete_non_branch_node_from_bdd(ep, bdd, target->get_id(), trash);
    }
  }

  BDD *branch_bdd_on_cache_write_success(
      const EP *ep, const Node *dchain_allocate_new_index,
      const fcfs_cached_table_data_t &cached_table_data,
      klee::ref<klee::Expr> cache_write_success_condition,
      const map_coalescing_objs_t &map_objs, Node *&on_cache_write_success,
      Node *&on_cache_write_failed,
      std::optional<constraints_t> &deleted_branch_constraints) const {
    const BDD *old_bdd = ep->get_bdd();
    BDD *new_bdd = new BDD(*old_bdd);

    const Node *next = dchain_allocate_new_index->get_next();
    assert(next);

    Branch *cache_write_branch;
    add_branch_to_bdd(ep, new_bdd, next, cache_write_success_condition,
                      cache_write_branch);
    on_cache_write_success = cache_write_branch->get_mutable_on_true();

    add_dchain_allocate_new_index_clone_on_cache_write_failed(
        ep, new_bdd, dchain_allocate_new_index, cache_write_branch,
        on_cache_write_failed);
    replicate_hdr_parsing_ops_on_cache_write_failed(
        ep, new_bdd, cache_write_branch, on_cache_write_failed);

    delete_coalescing_nodes_on_success(ep, new_bdd, on_cache_write_success,
                                       map_objs, cached_table_data.key,
                                       deleted_branch_constraints);

    return new_bdd;
  }
};

} // namespace tofino
