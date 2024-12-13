#pragma once

#include "tofino_cpu_module.h"

#include "hh_table_read.h"
#include "../tofino/hh_table_read.h"

namespace tofino_cpu {

using tofino::Table;

class HHTableUpdate : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> min_estimate;

public:
  HHTableUpdate(const Node *node, addr_t _obj,
                const std::vector<klee::ref<klee::Expr>> &_keys,
                klee::ref<klee::Expr> _value,
                klee::ref<klee::Expr> _min_estimate)
      : TofinoCPUModule(ModuleType::TofinoCPU_HHTableUpdate, "HHTableUpdate",
                        node),
        obj(_obj), keys(_keys), value(_value), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new HHTableUpdate(node, obj, keys, value, min_estimate);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  std::vector<klee::ref<klee::Expr>> get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }
};

class HHTableUpdateGenerator : public TofinoCPUModuleGenerator {
public:
  HHTableUpdateGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_HHTableUpdate,
                                 "HHTableUpdate") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::Call) {
      return std::nullopt;
    }

    const Call *map_put = static_cast<const Call *>(node);
    const call_t &call = map_put->get_call();

    if (call.function_name != "map_put") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
    addr_t obj = expr_addr_to_obj_addr(obj_expr);

    if (!ctx.check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
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

    const Call *map_put = static_cast<const Call *>(node);
    const call_t &call = map_put->get_call();

    if (call.function_name != "map_put") {
      return impls;
    }

    klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
    addr_t obj = expr_addr_to_obj_addr(obj_expr);

    if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_HeavyHitterTable)) {
      return impls;
    }

    klee::ref<klee::Expr> min_estimate = get_min_estimate(ep);
    ASSERT(!min_estimate.isNull(), "TODO: HHTableRead not found, so we should "
                                   "query the CMS for the min estimate");

    table_data_t table_data = get_table_data(map_put);

    Module *module =
        new HHTableUpdate(node, table_data.obj, table_data.table_keys,
                          table_data.value, min_estimate);
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
    klee::ref<klee::Expr> value;
  };

  table_data_t get_table_data(const Call *map_put) const {
    const call_t &call = map_put->get_call();
    ASSERT(call.function_name == "map_put", "Not a map_put call");

    klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> value = call.args.at("value").expr;

    addr_t obj = expr_addr_to_obj_addr(obj_expr);

    table_data_t table_data = {
        .obj = obj,
        .key = key,
        .table_keys = Table::build_keys(key),
        .value = value,
    };

    return table_data;
  }

  klee::ref<klee::Expr> get_min_estimate(const EP *ep) const {
    EPLeaf leaf = ep->get_active_leaf();
    const EPNode *node = leaf.node;

    while (node) {
      if (node->get_module()->get_type() == ModuleType::Tofino_HHTableRead) {
        const tofino::HHTableRead *hh_table_read =
            static_cast<const tofino::HHTableRead *>(node->get_module());
        return hh_table_read->get_min_estimate();
      } else if (node->get_module()->get_type() ==
                 ModuleType::TofinoCPU_HHTableRead) {
        const tofino_cpu::HHTableRead *hh_table_read =
            static_cast<const tofino_cpu::HHTableRead *>(node->get_module());
        return hh_table_read->get_min_estimate();
      }
      node = node->get_prev();
    }

    return nullptr;
  }
};

} // namespace tofino_cpu
