#pragma once

#include "tofino_module.h"

namespace tofino {

class FCFSCachedTableDelete : public TofinoModule {
private:
  DS_ID cached_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  symbol_t cached_delete_failed;

public:
  FCFSCachedTableDelete(const Node *node, DS_ID _cached_table_id, addr_t _obj,
                        klee::ref<klee::Expr> _key,
                        const symbol_t &_cached_delete_failed)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableDelete,
                     "FCFSCachedTableDelete", node),
        cached_table_id(_cached_table_id), obj(_obj), key(_key),
        cached_delete_failed(_cached_delete_failed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableDelete(node, cached_table_id, obj, key,
                                               cached_delete_failed);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  const symbol_t &get_cached_delete_failed() const {
    return cached_delete_failed;
  }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {cached_table_id};
  }
};

class FCFSCachedTableDeleteGenerator : public TofinoModuleGenerator {
public:
  FCFSCachedTableDeleteGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_FCFSCachedTableDelete,
                              "FCFSCachedTableDelete") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::Call) {
      return std::nullopt;
    }

    const Call *map_erase = static_cast<const Call *>(node);
    const call_t &call = map_erase->get_call();

    if (call.function_name != "map_erase") {
      return std::nullopt;
    }

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_map_op(ep, map_erase, map_objs)) {
      return std::nullopt;
    }

    if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
        !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable) ||
        !ctx.can_impl_ds(map_objs.vector_key, DSImpl::Tofino_FCFSCachedTable)) {
      return std::nullopt;
    }

    fcfs_cached_table_data_t cached_table_data =
        get_cached_table_data(ep, map_erase);

    std::vector<u32> allowed_cache_capacities =
        enum_fcfs_cache_cap(cached_table_data.num_entries);

    hit_rate_t chosen_cache_success_probability = 0;
    u32 chosen_cache_capacity = 0;
    bool successfully_placed = false;

    // We can use a different method for picking the right estimation depending
    // on the time it takes to find a solution.
    for (u32 cache_capacity : allowed_cache_capacities) {
      hit_rate_t cache_success_probability = get_cache_op_success_probability(
          ep, node, cached_table_data.key, cache_capacity);

      if (!can_get_or_build_fcfs_cached_table(
              ep, node, cached_table_data.obj, cached_table_data.key,
              cached_table_data.num_entries, cache_capacity)) {
        break;
      }

      if (cache_success_probability > chosen_cache_success_probability) {
        chosen_cache_success_probability = cache_success_probability;
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
    hit_rate_t on_fail_fraction =
        fraction * (1 - chosen_cache_success_probability);

    // FIXME: not using profiler cache.
    constraints_t constraints = node->get_ordered_branch_constraints();

    new_ctx.get_mutable_profiler().scale(constraints,
                                         chosen_cache_success_probability);
    new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
    new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);
    new_ctx.save_ds_impl(map_objs.vector_key, DSImpl::Tofino_FCFSCachedTable);

    new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_fraction);

    spec_impl_t spec_impl(
        decide(ep, node, {{CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

    std::vector<const Node *> ignore_nodes =
        get_future_related_nodes(ep, node, map_objs);

    for (const Node *op : ignore_nodes) {
      spec_impl.skip.insert(op->get_id());
    }

    return spec_impl;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::Call) {
      return impls;
    }

    const Call *map_erase = static_cast<const Call *>(node);
    const call_t &call = map_erase->get_call();

    if (call.function_name != "map_erase") {
      return impls;
    }

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_map_op(ep, map_erase, map_objs)) {
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

    fcfs_cached_table_data_t cached_table_data =
        get_cached_table_data(ep, map_erase);

    symbol_t cache_delete_failed = create_symbol("cache_delete_failed", 32);

    std::vector<u32> allowed_cache_capacities =
        enum_fcfs_cache_cap(cached_table_data.num_entries);

    for (u32 cache_capacity : allowed_cache_capacities) {
      std::optional<impl_t> impl = concretize_cached_table_delete(
          ep, map_erase, map_objs, cached_table_data, cache_delete_failed,
          cache_capacity);

      if (impl.has_value()) {
        impls.push_back(*impl);
      }
    }

    return impls;
  }

private:
  struct fcfs_cached_table_data_t {
    addr_t obj;
    klee::ref<klee::Expr> key;
    u32 num_entries;
  };

  std::optional<impl_t> concretize_cached_table_delete(
      const EP *ep, const Call *map_erase,
      const map_coalescing_objs_t &map_objs,
      const fcfs_cached_table_data_t &cached_table_data,
      const symbol_t &cache_delete_failed, u32 cache_capacity) const {
    FCFSCachedTable *cached_table = build_or_reuse_fcfs_cached_table(
        ep, map_erase, cached_table_data.obj, cached_table_data.key,
        cached_table_data.num_entries, cache_capacity);

    if (!cached_table) {
      return std::nullopt;
    }

    klee::ref<klee::Expr> cache_delete_success_condition =
        build_cache_delete_success_condition(cache_delete_failed);

    Module *module = new FCFSCachedTableDelete(
        map_erase, cached_table->id, cached_table_data.obj,
        cached_table_data.key, cache_delete_failed);
    EPNode *cached_table_delete_node = new EPNode(module);

    EP *new_ep = new EP(*ep);

    Node *on_cache_delete_success;
    Node *on_cache_delete_failed;
    std::optional<constraints_t> deleted_branch_constraints;

    BDD *new_bdd = branch_bdd_on_cache_delete_success(
        new_ep, map_erase, cached_table_data, cache_delete_success_condition,
        on_cache_delete_success, on_cache_delete_failed,
        deleted_branch_constraints);

    symbols_t symbols = get_dataplane_state(ep, map_erase);

    Module *if_module = new If(map_erase, cache_delete_success_condition,
                               {cache_delete_success_condition});
    Module *then_module = new Then(map_erase);
    Module *else_module = new Else(map_erase);
    Module *send_to_controller_module =
        new SendToController(on_cache_delete_failed, symbols);

    EPNode *if_node = new EPNode(if_module);
    EPNode *then_node = new EPNode(then_module);
    EPNode *else_node = new EPNode(else_module);
    EPNode *send_to_controller_node = new EPNode(send_to_controller_module);

    cached_table_delete_node->set_children(if_node);

    if_node->set_constraint(cache_delete_success_condition);
    if_node->set_prev(cached_table_delete_node);
    if_node->set_children(then_node, else_node);

    then_node->set_prev(if_node);

    else_node->set_prev(if_node);
    else_node->set_children(send_to_controller_node);

    send_to_controller_node->set_prev(else_node);

    hit_rate_t cache_success_probability = get_cache_op_success_probability(
        ep, map_erase, cached_table_data.key, cache_capacity);

    new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(
        new_ep->get_active_leaf().node->get_constraints(),
        cache_delete_success_condition, cache_success_probability);

    if (deleted_branch_constraints.has_value()) {
      new_ep->get_mutable_ctx().get_mutable_profiler().remove(
          deleted_branch_constraints.value());
    }

    Context &ctx = new_ep->get_mutable_ctx();
    ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
    ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);
    ctx.save_ds_impl(map_objs.vector_key, DSImpl::Tofino_FCFSCachedTable);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->place(new_ep, map_erase, map_objs.map, cached_table);

    EPLeaf on_cache_delete_success_leaf(then_node, on_cache_delete_success);
    EPLeaf on_cache_delete_failed_leaf(send_to_controller_node,
                                       on_cache_delete_failed);

    new_ep->process_leaf(
        cached_table_delete_node,
        {on_cache_delete_success_leaf, on_cache_delete_failed_leaf});
    new_ep->replace_bdd(new_bdd);
    new_ep->assert_integrity();

    new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(
        get_node_egress(new_ep, send_to_controller_node));

    return implement(ep, map_erase, new_ep,
                     {{CACHE_SIZE_PARAM, cache_capacity}});
  }

  klee::ref<klee::Expr> build_cache_delete_success_condition(
      const symbol_t &cache_delete_failed) const {
    klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(
        0, cache_delete_failed.expr->getWidth());
    return solver_toolbox.exprBuilder->Eq(cache_delete_failed.expr, zero);
  }

  hit_rate_t get_cache_op_success_probability(const EP *ep, const Node *node,
                                              klee::ref<klee::Expr> key,
                                              u32 cache_capacity) const {
    const Context &ctx = ep->get_ctx();
    const Profiler &profiler = ctx.get_profiler();

    hit_rate_t fraction = profiler.get_hr(node);
    hit_rate_t cache_hit_rate =
        get_fcfs_cache_success_rate(ep->get_ctx(), node, key, cache_capacity);

    return cache_hit_rate;
  }

  fcfs_cached_table_data_t get_cached_table_data(const EP *ep,
                                                 const Call *map_erase) const {
    fcfs_cached_table_data_t cached_table_data;

    const call_t &call = map_erase->get_call();
    assert(call.function_name == "map_erase");

    klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;

    cached_table_data.obj = expr_addr_to_obj_addr(map_addr_expr);
    cached_table_data.key = key;

    const Context &ctx = ep->get_ctx();
    const map_config_t &cfg = ctx.get_map_config(cached_table_data.obj);

    cached_table_data.num_entries = cfg.capacity;

    return cached_table_data;
  }

  std::vector<const Node *> get_future_related_nodes(
      const EP *ep, const Node *node,
      const map_coalescing_objs_t &cached_table_data) const {
    std::vector<const Call *> ops =
        get_future_functions(node, {"dchain_free_index", "map_erase"});

    std::vector<const Node *> related_ops;
    for (const Call *op : ops) {
      const call_t &call = op->get_call();

      if (call.function_name == "dchain_free_index") {
        klee::ref<klee::Expr> obj_expr = call.args.at("chain").expr;
        addr_t obj = expr_addr_to_obj_addr(obj_expr);

        if (obj != cached_table_data.dchain) {
          continue;
        }
      } else if (call.function_name == "map_erase") {
        klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
        addr_t obj = expr_addr_to_obj_addr(obj_expr);

        if (obj != cached_table_data.map) {
          continue;
        }
      } else {
        klee::ref<klee::Expr> obj_expr = call.args.at("vector").expr;
        addr_t obj = expr_addr_to_obj_addr(obj_expr);

        if (obj != cached_table_data.vector_key) {
          continue;
        }
      }

      related_ops.push_back(op);
    }

    return related_ops;
  }

  void replicate_hdr_parsing_ops_on_cache_delete_failed(
      const EP *ep, BDD *bdd, const Branch *cache_delete_branch,
      Node *&new_on_cache_delete_failed) const {
    const Node *on_cache_delete_failed = cache_delete_branch->get_on_false();

    std::vector<const Call *> prev_borrows = get_prev_functions(
        ep, on_cache_delete_failed, {"packet_borrow_next_chunk"});

    if (prev_borrows.empty()) {
      return;
    }

    std::vector<const Node *> non_branch_nodes_to_add;
    for (const Call *prev_borrow : prev_borrows) {
      non_branch_nodes_to_add.push_back(prev_borrow);
    }

    new_on_cache_delete_failed = add_non_branch_nodes_to_bdd(
        ep, bdd, on_cache_delete_failed, non_branch_nodes_to_add);
  }

  BDD *branch_bdd_on_cache_delete_success(
      const EP *ep, const Node *map_erase,
      const fcfs_cached_table_data_t &cached_table_data,
      klee::ref<klee::Expr> cache_delete_success_condition,
      Node *&on_cache_delete_success, Node *&on_cache_delete_failed,
      std::optional<constraints_t> &deleted_branch_constraints) const {
    const BDD *old_bdd = ep->get_bdd();
    BDD *new_bdd = new BDD(*old_bdd);

    const Node *next = map_erase->get_next();
    assert(next);

    Branch *cache_delete_branch =
        add_branch_to_bdd(ep, new_bdd, next, cache_delete_success_condition);

    on_cache_delete_success = cache_delete_branch->get_mutable_on_true();
    on_cache_delete_failed = cache_delete_branch->get_mutable_on_false();

    replicate_hdr_parsing_ops_on_cache_delete_failed(
        ep, new_bdd, cache_delete_branch, on_cache_delete_failed);

    return new_bdd;
  }
};

} // namespace tofino
