#pragma once

#include "tofino_module.h"

namespace tofino {

class TableLookup : public TofinoModule {
private:
  DS_ID table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<symbol_t> hit;

public:
  TableLookup(const Node *node, DS_ID _table_id, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              const std::vector<klee::ref<klee::Expr>> &_values,
              const std::optional<symbol_t> &_hit)
      : TofinoModule(ModuleType::Tofino_TableLookup, "TableLookup", node),
        table_id(_table_id), obj(_obj), keys(_keys), values(_values),
        hit(_hit) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TableLookup(node, table_id, obj, keys, values, hit);
    return cloned;
  }

  DS_ID get_table_id() const { return table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const std::vector<klee::ref<klee::Expr>> &get_values() const {
    return values;
  }
  const std::optional<symbol_t> &get_hit() const { return hit; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {table_id};
  }
};

class TableLookupGenerator : public TofinoModuleGenerator {
public:
  TableLookupGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_TableLookup, "TableLookup") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);

    if (is_vector_write(call_node)) {
      return std::nullopt;
    }

    std::optional<table_data_t> table_data = get_table_data(ep, call_node);

    if (!table_data.has_value()) {
      return std::nullopt;
    }

    if (!ctx.can_impl_ds(table_data->obj, DSImpl::Tofino_Table)) {
      return std::nullopt;
    }

    if (!can_build_table(ep, node, table_data.value())) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(table_data->obj, DSImpl::Tofino_Table);

    return spec_impl_t(decide(ep, node), new_ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *call_node = static_cast<const Call *>(node);

    if (is_vector_write(call_node)) {
      return impls;
    }

    std::optional<table_data_t> table_data = get_table_data(ep, call_node);

    if (!table_data.has_value()) {
      return impls;
    }

    if (!ep->get_ctx().can_impl_ds(table_data->obj, DSImpl::Tofino_Table)) {
      return impls;
    }

    Table *table = build_table(ep, node, table_data.value());

    if (!table) {
      return impls;
    }

    Module *module =
        new TableLookup(node, table->id, table_data->obj, table_data->keys,
                        table_data->values, table_data->hit);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    Context &ctx = new_ep->get_mutable_ctx();
    ctx.save_ds_impl(table_data->obj, DSImpl::Tofino_Table);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->place(new_ep, node, table_data->obj, table);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  std::optional<table_data_t> get_table_data(const EP *ep,
                                             const Call *call_node) const {
    const call_t &call = call_node->get_call();

    if (call.function_name == "map_get") {
      return table_data_from_map_op(ep, call_node);
    }

    if (call.function_name == "vector_borrow") {
      if (is_vector_map_key_function(ep, call_node) ||
          !is_vector_read(call_node)) {
        return std::nullopt;
      }

      return table_data_from_vector_op(ep, call_node);
    }

    if (call.function_name == "dchain_is_index_allocated" ||
        call.function_name == "dchain_rejuvenate_index") {
      return table_data_from_dchain_op(ep, call_node);
    }

    return std::nullopt;
  }

  table_data_t table_data_from_map_op(const EP *ep,
                                      const Call *call_node) const {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "map_get");

    klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> value_out = call.args.at("value_out").out;

    symbols_t symbols = call_node->get_locally_generated_symbols();

    symbol_t map_has_this_key;
    bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
    assert(found && "Symbol map_has_this_key not found");

    addr_t obj = expr_addr_to_obj_addr(map_addr_expr);

    const Context &ctx = ep->get_ctx();
    const map_config_t &cfg = ctx.get_map_config(obj);

    table_data_t table_data = {
        .obj = expr_addr_to_obj_addr(map_addr_expr),
        .num_entries = static_cast<u32>(cfg.capacity),
        .keys = Table::build_keys(key),
        .values = {value_out},
        .hit = map_has_this_key,
    };

    return table_data;
  }

  table_data_t table_data_from_vector_op(const EP *ep,
                                         const Call *call_node) const {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "vector_borrow");

    klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;
    klee::ref<klee::Expr> cell = call.extra_vars.at("borrowed_cell").second;

    addr_t obj = expr_addr_to_obj_addr(vector_addr_expr);

    const Context &ctx = ep->get_ctx();
    const vector_config_t &cfg = ctx.get_vector_config(obj);

    table_data_t table_data = {
        .obj = obj,
        .num_entries = static_cast<u32>(cfg.capacity),
        .keys = {index},
        .values = {cell},
        .hit = std::nullopt,
    };

    return table_data;
  }

  table_data_t table_data_from_dchain_op(const EP *ep,
                                         const Call *call_node) const {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "dchain_is_index_allocated" ||
           call.function_name == "dchain_rejuvenate_index");

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;

    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

    const Context &ctx = ep->get_ctx();
    const dchain_config_t &cfg = ctx.get_dchain_config(dchain_addr);

    table_data_t table_data = {
        .obj = dchain_addr,
        .num_entries = static_cast<u32>(cfg.index_range),
        .keys = {index},
        .values = {},
        .hit = std::nullopt,
    };

    if (call.function_name == "dchain_is_index_allocated") {
      symbols_t symbols = call_node->get_locally_generated_symbols();
      symbol_t is_allocated;
      bool found =
          get_symbol(symbols, "dchain_is_index_allocated", is_allocated);
      assert(found && "Symbol dchain_is_index_allocated not found");

      table_data.hit = is_allocated;
    }

    return table_data;
  }
};

} // namespace tofino
