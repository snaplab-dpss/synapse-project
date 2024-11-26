#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

using namespace tofino;

class TableLookup : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<symbol_t> found;

public:
  TableLookup(const Node *node, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              const std::vector<klee::ref<klee::Expr>> &_values,
              const std::optional<symbol_t> &_found)
      : TofinoCPUModule(ModuleType::TofinoCPU_TableLookup, "TableLookup", node),
        obj(_obj), keys(_keys), values(_values), found(_found) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TableLookup(node, obj, keys, values, found);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const std::vector<klee::ref<klee::Expr>> &get_values() const {
    return values;
  }
  const std::optional<symbol_t> &get_found() const { return found; }
};

class TableLookupGenerator : public TofinoCPUModuleGenerator {
public:
  TableLookupGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_TableLookup,
                                 "TableLookup") {}

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

    addr_t obj;
    std::vector<klee::ref<klee::Expr>> keys;
    std::vector<klee::ref<klee::Expr>> values;
    std::optional<symbol_t> found;
    if (!get_table_lookup_data(call_node, obj, keys, values, found)) {
      return std::nullopt;
    }

    if (!ctx.can_impl_ds(obj, DSImpl::Tofino_Table)) {
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

    if (is_vector_write(call_node)) {
      return impls;
    }

    addr_t obj;
    std::vector<klee::ref<klee::Expr>> keys;
    std::vector<klee::ref<klee::Expr>> values;
    std::optional<symbol_t> found;
    if (!get_table_lookup_data(call_node, obj, keys, values, found)) {
      return impls;
    }

    if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_Table)) {
      return impls;
    }

    Module *module = new TableLookup(node, obj, keys, values, found);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  bool get_table_lookup_data(const Call *call_node, addr_t &obj,
                             std::vector<klee::ref<klee::Expr>> &keys,
                             std::vector<klee::ref<klee::Expr>> &values,
                             std::optional<symbol_t> &hit) const {
    const call_t &call = call_node->get_call();

    if (call.function_name == "map_get") {
      table_data_from_map_op(call_node, obj, keys, values, hit);
    } else if (call.function_name == "vector_borrow") {
      table_data_from_vector_op(call_node, obj, keys, values, hit);
    } else if (call.function_name == "dchain_is_index_allocated" ||
               call.function_name == "dchain_rejuvenate_index") {
      table_data_from_dchain_op(call_node, obj, keys, values, hit);
    } else {
      return false;
    }

    return true;
  }

  void table_data_from_map_op(const Call *call_node, addr_t &obj,
                              std::vector<klee::ref<klee::Expr>> &keys,
                              std::vector<klee::ref<klee::Expr>> &values,
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
    values = {value_out};
    hit = map_has_this_key;
  }

  void table_data_from_vector_op(const Call *call_node, addr_t &obj,
                                 std::vector<klee::ref<klee::Expr>> &keys,
                                 std::vector<klee::ref<klee::Expr>> &values,
                                 std::optional<symbol_t> &hit) const {
    // We can implement even if we later update the vector's contents!

    const call_t &call = call_node->get_call();
    assert(call.function_name == "vector_borrow");

    klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;
    klee::ref<klee::Expr> cell = call.extra_vars.at("borrowed_cell").second;

    obj = expr_addr_to_obj_addr(vector_addr_expr);
    keys = {index};
    values = {cell};
  }

  void table_data_from_dchain_op(const Call *call_node, addr_t &obj,
                                 std::vector<klee::ref<klee::Expr>> &keys,
                                 std::vector<klee::ref<klee::Expr>> &values,
                                 std::optional<symbol_t> &hit) const {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "dchain_is_index_allocated" ||
           call.function_name == "dchain_rejuvenate_index");

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;

    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

    obj = dchain_addr;
    keys = {index};

    if (call.function_name == "dchain_is_index_allocated") {
      symbols_t symbols = call_node->get_locally_generated_symbols();
      symbol_t is_allocated;
      bool found =
          get_symbol(symbols, "dchain_is_index_allocated", is_allocated);
      assert(found && "Symbol dchain_is_index_allocated not found");

      hit = is_allocated;
    }
  }
};

} // namespace tofino_cpu
