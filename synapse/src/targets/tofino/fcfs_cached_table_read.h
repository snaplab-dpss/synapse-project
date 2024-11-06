#pragma once

#include "tofino_module.h"

namespace tofino {

class FCFSCachedTableRead : public TofinoModule {
private:
  DS_ID cached_table_id;
  std::unordered_set<DS_ID> cached_table_bydecisions;

  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  symbol_t map_has_this_key;

public:
  FCFSCachedTableRead(
      const Node *node, DS_ID _cached_table_id,
      const std::unordered_set<DS_ID> &_cached_table_bydecisions, addr_t _obj,
      klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value,
      const symbol_t &_map_has_this_key)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableRead,
                     "FCFSCachedTableRead", node),
        cached_table_id(_cached_table_id),
        cached_table_bydecisions(_cached_table_bydecisions), obj(_obj),
        key(_key), value(_value), map_has_this_key(_map_has_this_key) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new FCFSCachedTableRead(node, cached_table_id, cached_table_bydecisions,
                                obj, key, value, map_has_this_key);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return cached_table_bydecisions;
  }
};

class FCFSCachedTableReadGenerator : public TofinoModuleGenerator {
public:
  FCFSCachedTableReadGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_FCFSCachedTableRead,
                              "FCFSCachedTableRead") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *map_get = static_cast<const Call *>(node);
    const call_t &call = map_get->get_call();

    if (call.function_name != "map_get") {
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
        get_cached_table_data(ep, map_get);

    FCFSCachedTable *fcfs_cached_table =
        get_fcfs_cached_table(ep, node, cached_table_data.obj);
    if (!fcfs_cached_table) {
      return std::nullopt;
    }

    std::vector<const Call *> vector_ops =
        get_future_vector_key_ops(ep, node, cached_table_data, map_objs);

    Context new_ctx = ctx;

    new_ctx.save_ds_impl(cached_table_data.obj, DSImpl::Tofino_FCFSCachedTable);

    spec_impl_t spec_impl(
        decide(ep, node,
               {{CACHE_SIZE_PARAM, fcfs_cached_table->cache_capacity}}),
        new_ctx);

    for (const Call *vector_op : vector_ops) {
      spec_impl.skip.insert(vector_op->get_id());
    }

    return spec_impl;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *map_get = static_cast<const Call *>(node);
    const call_t &call = map_get->get_call();

    if (call.function_name != "map_get") {
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

    fcfs_cached_table_data_t cached_table_data =
        get_cached_table_data(ep, map_get);

    std::vector<int> allowed_cache_capacities =
        enum_fcfs_cache_cap(cached_table_data.num_entries);

    for (int cache_capacity : allowed_cache_capacities) {
      std::optional<impl_t> impl = concretize_cached_table_read(
          ep, node, map_objs, cached_table_data, cache_capacity);

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
    symbol_t map_has_this_key;
    int num_entries;
  };

  std::optional<impl_t> concretize_cached_table_read(
      const EP *ep, const Node *node, const map_coalescing_objs_t &map_objs,
      const fcfs_cached_table_data_t &cached_table_data,
      int cache_capacity) const {
    FCFSCachedTable *cached_table = build_or_reuse_fcfs_cached_table(
        ep, node, cached_table_data.obj, cached_table_data.key,
        cached_table_data.num_entries, cache_capacity);

    if (!cached_table) {
      return std::nullopt;
    }

    std::unordered_set<DS_ID> bydecisions =
        get_cached_table_bydecisions(cached_table);

    Module *module = new FCFSCachedTableRead(
        node, cached_table->id, bydecisions, cached_table_data.obj,
        cached_table_data.key, cached_table_data.read_value,
        cached_table_data.map_has_this_key);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);

    const Node *new_next;
    BDD *bdd = delete_future_vector_key_ops(new_ep, node, cached_table_data,
                                            map_objs, new_next);

    place_fcfs_cached_table(new_ep, node, map_objs, cached_table);

    EPLeaf leaf(ep_node, new_next);
    new_ep->process_leaf(ep_node, {leaf});
    new_ep->replace_bdd(bdd);
    // new_ep->inspect();

    return implement(ep, node, new_ep, {{CACHE_SIZE_PARAM, cache_capacity}});
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

  fcfs_cached_table_data_t get_cached_table_data(const EP *ep,
                                                 const Call *map_get) const {
    fcfs_cached_table_data_t cached_table_data;

    const call_t &call = map_get->get_call();
    symbols_t symbols = map_get->get_locally_generated_symbols();
    klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;

    cached_table_data.obj = expr_addr_to_obj_addr(obj_expr);
    cached_table_data.key = call.args.at("key").in;
    cached_table_data.read_value = call.args.at("value_out").out;

    bool found = get_symbol(symbols, "map_has_this_key",
                            cached_table_data.map_has_this_key);
    assert(found && "Symbol map_has_this_key not found");

    const Context &ctx = ep->get_ctx();
    const map_config_t &cfg = ctx.get_map_config(cached_table_data.obj);

    cached_table_data.num_entries = cfg.capacity;

    return cached_table_data;
  }

  std::vector<const Call *>
  get_future_vector_key_ops(const EP *ep, const Node *node,
                            const fcfs_cached_table_data_t &cached_table_data,
                            const map_coalescing_objs_t &map_objs) const {
    std::vector<const Call *> vector_ops =
        get_future_functions(node, {"vector_borrow", "vector_return"});

    for (const Call *vector_op : vector_ops) {
      const call_t &call = vector_op->get_call();

      klee::ref<klee::Expr> vector = call.args.at("vector").expr;
      klee::ref<klee::Expr> index = call.args.at("index").expr;

      addr_t vector_addr = expr_addr_to_obj_addr(vector);

      if (vector_addr != map_objs.vector_key) {
        continue;
      }

      if (!solver_toolbox.are_exprs_always_equal(
              index, cached_table_data.read_value)) {
        continue;
      }

      vector_ops.push_back(vector_op);
    }

    return vector_ops;
  }

  BDD *delete_future_vector_key_ops(
      EP *ep, const Node *node,
      const fcfs_cached_table_data_t &cached_table_data,
      const map_coalescing_objs_t &map_objs, const Node *&new_next) const {
    const BDD *old_bdd = ep->get_bdd();
    BDD *new_bdd = new BDD(*old_bdd);

    const Node *next = node->get_next();

    if (next) {
      new_next = new_bdd->get_node_by_id(next->get_id());
    } else {
      new_next = nullptr;
    }

    std::vector<const Call *> vector_ops =
        get_future_vector_key_ops(ep, node, cached_table_data, map_objs);

    for (const Call *vector_op : vector_ops) {
      bool replace_next = (vector_op == next);
      Node *replacement;
      delete_non_branch_node_from_bdd(ep, new_bdd, vector_op->get_id(),
                                      replacement);

      if (replace_next) {
        new_next = replacement;
      }
    }

    return new_bdd;
  }
};

} // namespace tofino
