#pragma once

#include "tofino_module.h"

namespace tofino {

class MapRegisterRead : public TofinoModule {
private:
  DS_ID map_register_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  symbol_t map_has_this_key;

public:
  MapRegisterRead(const Node *node, DS_ID _map_register_id, addr_t _obj,
                  klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value,
                  const symbol_t &_map_has_this_key)
      : TofinoModule(ModuleType::Tofino_MapRegisterRead, "MapRegisterRead",
                     node),
        map_register_id(_map_register_id), obj(_obj), key(_key), value(_value),
        map_has_this_key(_map_has_this_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MapRegisterRead(node, map_register_id, obj, key, value,
                                         map_has_this_key);
    return cloned;
  }

  DS_ID get_map_register_id() const { return map_register_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {map_register_id};
  }
};

class MapRegisterReadGenerator : public TofinoModuleGenerator {
public:
  MapRegisterReadGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_MapRegisterRead,
                              "MapRegisterRead") {}

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

    if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
        !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister) ||
        !ctx.can_impl_ds(map_objs.vector_key, DSImpl::Tofino_MapRegister)) {
      return std::nullopt;
    }

    map_register_data_t map_register_data = get_map_register_data(ep, map_get);

    if (!can_build_or_reuse_map_register(ep, node, map_register_data.obj,
                                         map_register_data.key,
                                         map_register_data.num_entries)) {
      return std::nullopt;
    }

    std::vector<const Call *> vector_ops =
        get_future_vector_key_ops(ep, node, map_register_data, map_objs);

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
    new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);
    new_ctx.save_ds_impl(map_objs.vector_key, DSImpl::Tofino_MapRegister);

    spec_impl_t spec_impl(decide(ep, node), new_ctx);

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

    if (!ep->get_ctx().can_impl_ds(map_objs.map, DSImpl::Tofino_MapRegister) ||
        !ep->get_ctx().can_impl_ds(map_objs.dchain,
                                   DSImpl::Tofino_MapRegister) ||
        !ep->get_ctx().can_impl_ds(map_objs.vector_key,
                                   DSImpl::Tofino_MapRegister)) {
      return impls;
    }

    map_register_data_t map_register_data = get_map_register_data(ep, map_get);

    MapRegister *map_register = build_or_reuse_map_register(
        ep, node, map_register_data.obj, map_register_data.key,
        map_register_data.num_entries);

    if (!map_register) {
      return impls;
    }

    Module *module = new MapRegisterRead(
        node, map_register->id, map_register_data.obj, map_register_data.key,
        map_register_data.read_value, map_register_data.map_has_this_key);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    const Node *new_next;
    BDD *bdd = delete_future_vector_key_ops(new_ep, node, map_register_data,
                                            map_objs, new_next);

    Context &ctx = new_ep->get_mutable_ctx();
    ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
    ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);
    ctx.save_ds_impl(map_objs.vector_key, DSImpl::Tofino_MapRegister);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->place(new_ep, node, map_objs.map, map_register);

    EPLeaf leaf(ep_node, new_next);
    new_ep->process_leaf(ep_node, {leaf});
    new_ep->replace_bdd(bdd);
    new_ep->assert_integrity();

    return impls;
  }

private:
  struct map_register_data_t {
    addr_t obj;
    klee::ref<klee::Expr> key;
    klee::ref<klee::Expr> read_value;
    symbol_t map_has_this_key;
    u32 num_entries;
  };

  map_register_data_t get_map_register_data(const EP *ep,
                                            const Call *map_get) const {
    map_register_data_t map_register_data;

    const call_t &call = map_get->get_call();
    symbols_t symbols = map_get->get_locally_generated_symbols();
    klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;

    map_register_data.obj = expr_addr_to_obj_addr(obj_expr);
    map_register_data.key = call.args.at("key").in;
    map_register_data.read_value = call.args.at("value_out").out;

    bool found = get_symbol(symbols, "map_has_this_key",
                            map_register_data.map_has_this_key);
    assert(found && "Symbol map_has_this_key not found");

    const Context &ctx = ep->get_ctx();
    const map_config_t &cfg = ctx.get_map_config(map_register_data.obj);

    map_register_data.num_entries = cfg.capacity;

    return map_register_data;
  }

  std::vector<const Call *>
  get_future_vector_key_ops(const EP *ep, const Node *node,
                            const map_register_data_t &map_register_data,
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
              index, map_register_data.read_value)) {
        continue;
      }

      vector_ops.push_back(vector_op);
    }

    return vector_ops;
  }

  BDD *delete_future_vector_key_ops(
      EP *ep, const Node *node, const map_register_data_t &map_register_data,
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
        get_future_vector_key_ops(ep, new_next, map_register_data, map_objs);

    for (const Call *vector_op : vector_ops) {
      bool replace_next = (vector_op == next);
      Node *replacement =
          delete_non_branch_node_from_bdd(ep, new_bdd, vector_op->get_id());

      if (replace_next) {
        new_next = replacement;
      }
    }

    return new_bdd;
  }
};

} // namespace tofino
