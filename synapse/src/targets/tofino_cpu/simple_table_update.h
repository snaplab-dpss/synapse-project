#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class SimpleTableUpdate : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;

public:
  SimpleTableUpdate(const Node *node, addr_t _obj,
                    const std::vector<klee::ref<klee::Expr>> &_keys,
                    const std::vector<klee::ref<klee::Expr>> &_values)
      : TofinoCPUModule(ModuleType::TofinoCPU_SimpleTableUpdate,
                        "SimpleTableUpdate", node),
        obj(_obj), keys(_keys), values(_values) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    SimpleTableUpdate *cloned = new SimpleTableUpdate(node, obj, keys, values);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const std::vector<klee::ref<klee::Expr>> &get_values() const {
    return values;
  }

  std::vector<const tofino::Table *> get_tables(const EP *ep) {
    const Context &ctx = ep->get_ctx();
    const tofino::TofinoContext *tofino_ctx =
        ctx.get_target_ctx<tofino::TofinoContext>();
    const std::unordered_set<tofino::DS *> &data_structures =
        tofino_ctx->get_ds(obj);

    std::vector<const tofino::Table *> tables;
    for (const tofino::DS *data_structure : data_structures) {
      assert(data_structure->type == tofino::DSType::TABLE);
      const tofino::Table *table =
          static_cast<const tofino::Table *>(data_structure);
      tables.push_back(table);
    }

    return tables;
  }
};

class SimpleTableUpdateGenerator : public TofinoCPUModuleGenerator {
public:
  SimpleTableUpdateGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_SimpleTableUpdate,
                                 "SimpleTableUpdate") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);

    if (!can_place_simple_table_update(ep, call_node)) {
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

    if (!check_simple_table_update_placement(ep, call_node)) {
      return impls;
    }

    addr_t obj;
    std::vector<klee::ref<klee::Expr>> keys;
    std::vector<klee::ref<klee::Expr>> values;
    get_table_update_data(call_node, obj, keys, values);

    Module *module = new SimpleTableUpdate(node, obj, keys, values);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  bool check_simple_table_update_placement(const EP *ep,
                                           const Call *call_node) const {
    const call_t &call = call_node->get_call();

    std::string obj_arg;
    if (call.function_name == "map_put") {
      obj_arg = "map";
    } else if (call.function_name == "vector_return") {
      obj_arg = "vector";
    } else if (call.function_name == "dchain_allocate_new_index") {
      obj_arg = "chain";
    } else {
      return false;
    }

    return check_placement(ep, call_node, obj_arg,
                           PlacementDecision::Tofino_SimpleTable);
  }

  bool can_place_simple_table_update(const EP *ep,
                                     const Call *call_node) const {
    const call_t &call = call_node->get_call();

    std::string obj_arg;
    if (call.function_name == "map_put") {
      obj_arg = "map";
    } else if (call.function_name == "vector_return") {
      obj_arg = "vector";
    } else if (call.function_name == "dchain_allocate_new_index") {
      obj_arg = "chain";
    } else {
      return false;
    }

    return can_place(ep, call_node, obj_arg,
                     PlacementDecision::Tofino_SimpleTable);
  }

  void get_table_update_data(const Call *call_node, addr_t &obj,
                             std::vector<klee::ref<klee::Expr>> &keys,
                             std::vector<klee::ref<klee::Expr>> &values) const {
    const call_t &call = call_node->get_call();

    if (call.function_name == "map_put") {
      table_update_data_from_map_op(call_node, obj, keys, values);
    } else if (call.function_name == "vector_return") {
      table_update_data_from_vector_op(call_node, obj, keys, values);
    } else if (call.function_name == "dchain_allocate_new_index") {
      table_data_from_dchain_op(call_node, obj, keys, values);
    } else {
      assert(false && "Unknown call");
    }
  }

  void table_update_data_from_map_op(
      const Call *call_node, addr_t &obj,
      std::vector<klee::ref<klee::Expr>> &keys,
      std::vector<klee::ref<klee::Expr>> &values) const {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "map_put");

    klee::ref<klee::Expr> map_addr_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> value = call.args.at("value").expr;

    obj = expr_addr_to_obj_addr(map_addr_expr);
    keys = Table::build_keys(key);
    values = {value};
  }

  void table_update_data_from_vector_op(
      const Call *call_node, addr_t &obj,
      std::vector<klee::ref<klee::Expr>> &keys,
      std::vector<klee::ref<klee::Expr>> &values) const {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "vector_return");

    klee::ref<klee::Expr> vector_addr_expr = call.args.at("vector").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;
    klee::ref<klee::Expr> value = call.args.at("value").in;

    obj = expr_addr_to_obj_addr(vector_addr_expr);
    keys = {index};
    values = {value};
  }

  void
  table_data_from_dchain_op(const Call *call_node, addr_t &obj,
                            std::vector<klee::ref<klee::Expr>> &keys,
                            std::vector<klee::ref<klee::Expr>> &values) const {
    const call_t &call = call_node->get_call();
    assert(call.function_name == "dchain_allocate_new_index");

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    klee::ref<klee::Expr> index_out = call.args.at("index_out").out;

    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

    obj = dchain_addr;
    keys = {index_out};
    // No value, the index is actually the table key
  }
};

} // namespace tofino_cpu
