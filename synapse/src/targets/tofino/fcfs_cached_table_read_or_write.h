#pragma once

#include "tofino_module.h"

#include "if.h"
#include "then.h"
#include "else.h"
#include "send_to_controller.h"

namespace tofino {

class FCFSCachedTableReadOrWrite : public TofinoModule {
private:
  DS_ID cached_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;
  symbol_t cache_write_failed;

public:
  FCFSCachedTableReadOrWrite(const Node *node, DS_ID _cached_table_id,
                             addr_t _obj, klee::ref<klee::Expr> _key,
                             klee::ref<klee::Expr> _read_value,
                             klee::ref<klee::Expr> _write_value,
                             const symbol_t &_map_has_this_key,
                             const symbol_t &_cache_write_failed)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableReadOrWrite,
                     "FCFSCachedTableReadOrWrite", node),
        cached_table_id(_cached_table_id), obj(_obj), key(_key),
        read_value(_read_value), write_value(_write_value),
        map_has_this_key(_map_has_this_key),
        cache_write_failed(_cache_write_failed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableReadOrWrite(
        node, cached_table_id, obj, key, read_value, write_value,
        map_has_this_key, cache_write_failed);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }
  const symbol_t &get_cache_write_failed() const { return cache_write_failed; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {cached_table_id};
  }
};

class FCFSCachedTableReadOrWriteGenerator : public TofinoModuleGenerator {
public:
  FCFSCachedTableReadOrWriteGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_FCFSCachedTableReadOrWrite,
                              "FCFSCachedTableReadOrWrite") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::Call) {
      return std::nullopt;
    }

    const Call *map_get = static_cast<const Call *>(node);

    std::vector<const Call *> future_map_puts;
    if (!is_map_get_followed_by_map_puts_on_miss(ep->get_bdd(), map_get,
                                                 future_map_puts)) {
      // The cached table read should deal with these cases.
      return std::nullopt;
    }

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
      return std::nullopt;
    }

    if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_FCFSCachedTable) ||
        !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable) ||
        !ctx.can_impl_ds(map_objs.vector_key, DSImpl::Tofino_FCFSCachedTable)) {
      return std::nullopt;
    }

    fcfs_cached_table_data_t cached_table_data =
        get_fcfs_cached_table_data(ep, map_get, future_map_puts);

    std::vector<u32> allowed_cache_capacities =
        enum_fcfs_cache_cap(cached_table_data.num_entries);

    hit_rate_t chosen_success_probability = 0;
    u32 chosen_cache_capacity = 0;
    bool successfully_placed = false;

    // We can use a different method for picking the right estimation depending
    // on the time it takes to find a solution.
    for (u32 cache_capacity : allowed_cache_capacities) {
      hit_rate_t success_probability = get_cache_op_success_probability(
          ep, node, cached_table_data.key, cache_capacity);

      if (!can_get_or_build_fcfs_cached_table(
              ep, node, cached_table_data.obj, cached_table_data.key,
              cached_table_data.num_entries, cache_capacity)) {
        break;
      }

      if (success_probability > chosen_success_probability) {
        chosen_success_probability = success_probability;
        chosen_cache_capacity = cache_capacity;
      }

      successfully_placed = true;
    }

    if (!successfully_placed) {
      return std::nullopt;
    }

    Context new_ctx = ctx;

    hit_rate_t hr = new_ctx.get_profiler().get_hr(node);
    hit_rate_t on_fail_hr = hr * (1 - chosen_success_probability);

    // FIXME: not using profiler cache.
    constraints_t constraints = node->get_ordered_branch_constraints();
    new_ctx.get_mutable_profiler().scale(constraints,
                                         chosen_success_probability);
    new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
    new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);
    new_ctx.save_ds_impl(map_objs.vector_key, DSImpl::Tofino_FCFSCachedTable);

    new_ctx.get_mutable_perf_oracle().add_controller_traffic(on_fail_hr);

    spec_impl_t spec_impl(
        decide(ep, node, {{CACHE_SIZE_PARAM, chosen_cache_capacity}}), new_ctx);

    std::vector<const Node *> ignore_nodes = get_nodes_to_speculatively_ignore(
        ep, map_get, map_objs, cached_table_data.key);

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

    const Call *map_get = static_cast<const Call *>(node);

    std::vector<const Call *> future_map_puts;
    if (!is_map_get_followed_by_map_puts_on_miss(ep->get_bdd(), map_get,
                                                 future_map_puts)) {
      // The cached table read should deal with these cases.
      return impls;
    }

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_map_op(ep, map_get, map_objs)) {
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
        get_fcfs_cached_table_data(ep, map_get, future_map_puts);

    std::vector<u32> allowed_cache_capacities =
        enum_fcfs_cache_cap(cached_table_data.num_entries);

    for (u32 cache_capacity : allowed_cache_capacities) {
      std::optional<impl_t> impl = concretize_cached_table_cond_write(
          ep, node, map_objs, cached_table_data, cache_write_failed,
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
    klee::ref<klee::Expr> read_value;
    klee::ref<klee::Expr> write_value;
    symbol_t map_has_this_key;
    u32 num_entries;
  };

  std::optional<impl_t> concretize_cached_table_cond_write(
      const EP *ep, const Node *node, const map_coalescing_objs_t &map_objs,
      const fcfs_cached_table_data_t &fcfs_cached_table_data,
      const symbol_t &cache_write_failed, u32 cache_capacity) const {
    FCFSCachedTable *cached_table = build_or_reuse_fcfs_cached_table(
        ep, node, fcfs_cached_table_data.obj, fcfs_cached_table_data.key,
        fcfs_cached_table_data.num_entries, cache_capacity);

    if (!cached_table) {
      return std::nullopt;
    }

    klee::ref<klee::Expr> cache_write_success_condition =
        build_cache_write_success_condition(cache_write_failed);

    Module *module = new FCFSCachedTableReadOrWrite(
        node, cached_table->id, fcfs_cached_table_data.obj,
        fcfs_cached_table_data.key, fcfs_cached_table_data.read_value,
        fcfs_cached_table_data.write_value,
        fcfs_cached_table_data.map_has_this_key, cache_write_failed);
    EPNode *cached_table_cond_write_node = new EPNode(module);

    EP *new_ep = new EP(*ep);

    Node *on_cache_write_success;
    Node *on_cache_write_failed;
    std::optional<constraints_t> deleted_branch_constraints;

    BDD *new_bdd = branch_bdd_on_cache_write_success(
        new_ep, node, fcfs_cached_table_data, cache_write_success_condition,
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

    cached_table_cond_write_node->set_children(if_node);

    if_node->set_constraint(cache_write_success_condition);
    if_node->set_prev(cached_table_cond_write_node);
    if_node->set_children(then_node, else_node);

    then_node->set_prev(if_node);

    else_node->set_prev(if_node);
    else_node->set_children(send_to_controller_node);

    send_to_controller_node->set_prev(else_node);

    hit_rate_t cache_success_probability = get_cache_op_success_probability(
        ep, node, fcfs_cached_table_data.key, cache_capacity);

    new_ep->get_mutable_ctx().get_mutable_profiler().insert_relative(
        new_ep->get_active_leaf().node->get_constraints(),
        cache_write_success_condition, cache_success_probability);

    Context &ctx = new_ep->get_mutable_ctx();
    ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_FCFSCachedTable);
    ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_FCFSCachedTable);
    ctx.save_ds_impl(map_objs.vector_key, DSImpl::Tofino_FCFSCachedTable);

    if (deleted_branch_constraints.has_value()) {
      ctx.get_mutable_profiler().remove(deleted_branch_constraints.value());
    }

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->place(new_ep, node, map_objs.map, cached_table);

    EPLeaf on_cache_write_success_leaf(then_node, on_cache_write_success);
    EPLeaf on_cache_write_failed_leaf(send_to_controller_node,
                                      on_cache_write_failed);

    new_ep->process_leaf(
        cached_table_cond_write_node,
        {on_cache_write_success_leaf, on_cache_write_failed_leaf});
    new_ep->replace_bdd(new_bdd);
    new_ep->assert_integrity();

    new_ep->get_mutable_ctx().get_mutable_perf_oracle().add_controller_traffic(
        get_node_egress(new_ep, send_to_controller_node));

    return implement(ep, node, new_ep, {{CACHE_SIZE_PARAM, cache_capacity}});
  }

  hit_rate_t get_cache_op_success_probability(const EP *ep, const Node *node,
                                              klee::ref<klee::Expr> key,
                                              u32 cache_capacity) const {
    hit_rate_t hr = ep->get_ctx().get_profiler().get_hr(node);
    rw_fractions_t rw_hr = get_cond_map_put_rw_profile_fractions(ep, node);

    hit_rate_t failed_writes = rw_hr.write_attempt - rw_hr.write;

    hit_rate_t rp = rw_hr.read / hr;
    hit_rate_t wp = rw_hr.write / hr;
    hit_rate_t fwp = failed_writes / hr;

    hit_rate_t cache_hit_rate =
        get_fcfs_cache_success_rate(ep->get_ctx(), node, key, cache_capacity);

    hit_rate_t probability = rp + wp * cache_hit_rate;

    // if (node->get_id() == 14) {
    //   std::cerr << "rp: " << rp << std::endl;
    //   std::cerr << "wp: " << wp << std::endl;
    //   std::cerr << "fwp: " << fwp << std::endl;
    //   std::cerr << "cache_hit_rate: " << cache_hit_rate << std::endl;
    //   std::cerr << "success_probability: " << probability << std::endl;
    //   DEBUG_PAUSE
    // }

    return probability;
  }

  fcfs_cached_table_data_t
  get_fcfs_cached_table_data(const EP *ep, const Call *map_get,
                             std::vector<const Call *> future_map_puts) const {
    fcfs_cached_table_data_t cached_table_data;

    assert(!future_map_puts.empty());
    const Call *map_put = future_map_puts.front();

    const call_t &get_call = map_get->get_call();
    const call_t &put_call = map_put->get_call();

    symbols_t symbols = map_get->get_locally_generated_symbols();
    klee::ref<klee::Expr> obj_expr = get_call.args.at("map").expr;

    cached_table_data.obj = expr_addr_to_obj_addr(obj_expr);
    cached_table_data.key = get_call.args.at("key").in;
    cached_table_data.read_value = get_call.args.at("value_out").out;
    cached_table_data.write_value = put_call.args.at("value").expr;

    bool found = get_symbol(symbols, "map_has_this_key",
                            cached_table_data.map_has_this_key);
    assert(found && "Symbol map_has_this_key not found");

    const Context &ctx = ep->get_ctx();
    const map_config_t &cfg = ctx.get_map_config(cached_table_data.obj);

    cached_table_data.num_entries = cfg.capacity;
    return cached_table_data;
  }

  klee::ref<klee::Expr> build_cache_write_success_condition(
      const symbol_t &cache_write_failed) const {
    klee::ref<klee::Expr> zero = solver_toolbox.exprBuilder->Constant(
        0, cache_write_failed.expr->getWidth());
    return solver_toolbox.exprBuilder->Eq(cache_write_failed.expr, zero);
  }

  void add_map_get_clone_on_cache_write_failed(
      const EP *ep, BDD *bdd, const Node *map_get,
      const Branch *cache_write_branch,
      Node *&new_on_cache_write_failed) const {
    node_id_t &id = bdd->get_mutable_id();
    NodeManager &manager = bdd->get_mutable_manager();
    Node *map_get_on_cache_write_failed = map_get->clone(manager, false);
    map_get_on_cache_write_failed->recursive_update_ids(id);

    new_on_cache_write_failed =
        add_non_branch_nodes_to_bdd(ep, bdd, cache_write_branch->get_on_false(),
                                    {map_get_on_cache_write_failed});
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

    new_on_cache_write_failed = add_non_branch_nodes_to_bdd(
        ep, bdd, on_cache_write_failed, non_branch_nodes_to_add);
  }

  std::vector<const Node *>
  get_nodes_to_speculatively_ignore(const EP *ep, const Node *on_success,
                                    const map_coalescing_objs_t &map_objs,
                                    klee::ref<klee::Expr> key) const {
    std::vector<const Node *> nodes_to_ignore;

    const BDD *bdd = ep->get_bdd();
    std::vector<const Call *> coalescing_nodes =
        get_coalescing_nodes_from_key(bdd, on_success, key, map_objs);

    for (const Call *coalescing_node : coalescing_nodes) {
      nodes_to_ignore.push_back(coalescing_node);

      const call_t &call = coalescing_node->get_call();

      if (call.function_name == "dchain_allocate_new_index") {
        symbol_t out_of_space;
        bool found =
            get_symbol(coalescing_node->get_locally_generated_symbols(),
                       "out_of_space", out_of_space);
        assert(found && "Symbol out_of_space not found");

        index_alloc_check_t index_alloc_check =
            find_branch_checking_index_alloc(ep, on_success, out_of_space);

        // FIXME: We ignore all logic happening when the index is not
        // successfuly allocated. This is a major simplification, as the NF
        // might be doing something useful here. We never encountered a scenario
        // where this was the case, but it could happen nonetheless.
        if (index_alloc_check.branch) {
          nodes_to_ignore.push_back(index_alloc_check.branch);

          const Node *next = index_alloc_check.success_on_true
                                 ? index_alloc_check.branch->get_on_false()
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

        index_alloc_check_t index_alloc_check =
            find_branch_checking_index_alloc(ep, on_success, out_of_space);

        if (index_alloc_check.branch) {
          assert(!deleted_branch_constraints.has_value() &&
                 "Multiple branch checking index allocation detected");
          deleted_branch_constraints =
              index_alloc_check.branch->get_ordered_branch_constraints();

          klee::ref<klee::Expr> extra_constraint =
              index_alloc_check.branch->get_condition();

          // If we want to keep the direction on true, we must remove the on
          // false.
          if (index_alloc_check.success_on_true) {
            extra_constraint =
                solver_toolbox.exprBuilder->Not(extra_constraint);
          }

          deleted_branch_constraints->push_back(extra_constraint);

          Node *trash = delete_branch_node_from_bdd(
              ep, bdd, index_alloc_check.branch->get_id(),
              index_alloc_check.success_on_true);
        }
      }

      Node *trash = delete_non_branch_node_from_bdd(ep, bdd, target->get_id());
    }
  }

  BDD *branch_bdd_on_cache_write_success(
      const EP *ep, const Node *map_get,
      const fcfs_cached_table_data_t &fcfs_cached_table_data,
      klee::ref<klee::Expr> cache_write_success_condition,
      const map_coalescing_objs_t &map_objs, Node *&on_cache_write_success,
      Node *&on_cache_write_failed,
      std::optional<constraints_t> &deleted_branch_constraints) const {
    const BDD *old_bdd = ep->get_bdd();
    BDD *new_bdd = new BDD(*old_bdd);

    const Node *next = map_get->get_next();
    assert(next);

    Branch *cache_write_branch =
        add_branch_to_bdd(ep, new_bdd, next, cache_write_success_condition);

    on_cache_write_success = cache_write_branch->get_mutable_on_true();
    add_map_get_clone_on_cache_write_failed(
        ep, new_bdd, map_get, cache_write_branch, on_cache_write_failed);

    replicate_hdr_parsing_ops_on_cache_write_failed(
        ep, new_bdd, cache_write_branch, on_cache_write_failed);

    delete_coalescing_nodes_on_success(ep, new_bdd, on_cache_write_success,
                                       map_objs, fcfs_cached_table_data.key,
                                       deleted_branch_constraints);

    return new_bdd;
  }
};

} // namespace tofino
