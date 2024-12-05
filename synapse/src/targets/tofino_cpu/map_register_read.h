#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

using namespace tofino;

class MapRegisterRead : public TofinoCPUModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<symbol_t> found;

public:
  MapRegisterRead(const Node *node, DS_ID _id, addr_t _obj,
                  const std::vector<klee::ref<klee::Expr>> &_keys,
                  klee::ref<klee::Expr> _value,
                  const std::optional<symbol_t> &_found)
      : TofinoCPUModule(ModuleType::TofinoCPU_MapRegisterRead,
                        "MapRegisterRead", node),
        id(_id), obj(_obj), keys(_keys), value(_value), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MapRegisterRead(node, id, obj, keys, value, found);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const std::optional<symbol_t> &get_found() const { return found; }
};

class MapRegisterReadGenerator : public TofinoCPUModuleGenerator {
public:
  MapRegisterReadGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_MapRegisterRead,
                                 "MapRegisterRead") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "map_get") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
    addr_t map_addr = expr_addr_to_obj_addr(map_addr_expr);

    if (!ctx.can_impl_ds(map_addr, DSImpl::Tofino_MapRegister)) {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "map_get") {
      return impls;
    }

    addr_t obj;
    std::vector<klee::ref<klee::Expr>> keys;
    klee::ref<klee::Expr> value;
    std::optional<symbol_t> found;
    get_data(call_node, obj, keys, value, found);

    if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_MapRegister)) {
      return impls;
    }

    DS_ID id = get_map_register(ep, obj);

    Module *module = new MapRegisterRead(node, id, obj, keys, value, found);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  DS_ID get_map_register(const EP *ep, addr_t obj) const {
    const Context &ctx = ep->get_ctx();
    const tofino::TofinoContext *tofino_ctx =
        ctx.get_target_ctx<tofino::TofinoContext>();
    const std::unordered_set<tofino::DS *> &data_structures =
        tofino_ctx->get_ds(obj);
    assert(data_structures.size() == 1);
    tofino::DS *ds = *data_structures.begin();
    assert(ds->type == tofino::DSType::MAP_REGISTER);
    return ds->id;
  }

  void get_data(const Call *call_node, addr_t &obj,
                std::vector<klee::ref<klee::Expr>> &keys,
                klee::ref<klee::Expr> &value,
                std::optional<symbol_t> &hit) const {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "map_get");

    klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> value_out = call.args.at("value_out").out;

    symbols_t symbols = call_node->get_locally_generated_symbols();

    symbol_t map_has_this_key;
    bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
    assert(found && "Symbol map_has_this_key not found");

    obj = expr_addr_to_obj_addr(map_addr_expr);
    keys = Table::build_keys(key);
    value = value_out;
    hit = map_has_this_key;
  }
};

} // namespace tofino_cpu
