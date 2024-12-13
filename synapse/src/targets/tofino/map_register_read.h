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
    if (node->get_type() != NodeType::Call) {
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
        !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_MapRegister)) {
      return std::nullopt;
    }

    map_register_data_t map_register_data = get_map_register_data(ep, map_get);

    if (!can_build_or_reuse_map_register(ep, node, map_register_data.obj,
                                         map_register_data.key,
                                         map_register_data.num_entries)) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
    new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

    return spec_impl_t(decide(ep, node), new_ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::Call) {
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

    Context &ctx = new_ep->get_mutable_ctx();
    ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_MapRegister);
    ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_MapRegister);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->place(new_ep, node, map_objs.map, map_register);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

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
    ASSERT(found, "Symbol map_has_this_key not found");

    const Context &ctx = ep->get_ctx();
    const map_config_t &cfg = ctx.get_map_config(map_register_data.obj);

    map_register_data.num_entries = cfg.capacity;

    return map_register_data;
  }
};

} // namespace tofino
