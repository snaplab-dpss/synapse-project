#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

using tofino::DS;
using tofino::DS_ID;
using tofino::DSType;
using tofino::HHTable;
using tofino::Table;
using tofino::TofinoContext;

class HHTableRead : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> map_has_this_key;
  klee::ref<klee::Expr> min_estimate;

public:
  HHTableRead(const Node *node, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _value,
              klee::ref<klee::Expr> _map_has_this_key,
              klee::ref<klee::Expr> _min_estimate)
      : TofinoCPUModule(ModuleType::TofinoCPU_HHTableRead, "HHTableRead", node),
        obj(_obj), keys(_keys), value(_value),
        map_has_this_key(_map_has_this_key), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new HHTableRead(node, obj, keys, value, map_has_this_key, min_estimate);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_hit() const { return map_has_this_key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }
};

class HHTableReadGenerator : public TofinoCPUModuleGenerator {
public:
  HHTableReadGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_HHTableRead,
                                 "HHTableRead") {}

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

    if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
        !ctx.check_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable) ||
        !ctx.check_ds_impl(map_objs.vector_key,
                           DSImpl::Tofino_HeavyHitterTable)) {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
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

    if (!ep->get_ctx().check_ds_impl(map_objs.map,
                                     DSImpl::Tofino_HeavyHitterTable) ||
        !ep->get_ctx().check_ds_impl(map_objs.dchain,
                                     DSImpl::Tofino_HeavyHitterTable) ||
        !ep->get_ctx().check_ds_impl(map_objs.vector_key,
                                     DSImpl::Tofino_HeavyHitterTable)) {
      return impls;
    }

    table_data_t table_data = get_table_data(ep, map_get);

    Module *module = new HHTableRead(
        node, table_data.obj, table_data.table_keys, table_data.read_value,
        table_data.map_has_this_key, table_data.min_estimate);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  struct table_data_t {
    addr_t obj;
    klee::ref<klee::Expr> key;
    std::vector<klee::ref<klee::Expr>> table_keys;
    klee::ref<klee::Expr> read_value;
    klee::ref<klee::Expr> map_has_this_key;
    klee::ref<klee::Expr> min_estimate;
    int num_entries;
  };

  table_data_t get_table_data(const EP *ep, const Call *map_get) const {
    const call_t &call = map_get->get_call();
    assert(call.function_name == "map_get");

    symbols_t symbols = map_get->get_locally_generated_symbols();

    klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> value_out = call.args.at("value_out").out;

    symbol_t map_has_this_key;
    bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
    assert(found && "Symbol map_has_this_key not found");

    addr_t obj = expr_addr_to_obj_addr(obj_expr);
    klee::ref<klee::Expr> min_estimate = solver_toolbox.create_new_symbol(
        "min_estimate_" + std::to_string(map_get->get_id()), 32);
    int capacity = ep->get_ctx().get_map_config(obj).capacity;

    table_data_t table_data = {
        .obj = obj,
        .key = key,
        .table_keys = Table::build_keys(key),
        .read_value = value_out,
        .map_has_this_key = map_has_this_key.expr,
        .min_estimate = min_estimate,
        .num_entries = capacity,
    };

    return table_data;
  }
};

} // namespace tofino_cpu
