#pragma once

#include "tofino_cpu_module.h"

#include "if.h"
#include "then.h"
#include "else.h"
#include "../tofino/hh_table_read.h"
#include "hh_table_read.h"
#include "hh_table_update.h"

namespace tofino_cpu {

using tofino::HHTable;
using tofino::Table;

class HHTableConditionalUpdate : public TofinoCPUModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;
  symbol_t cache_write_failed;

public:
  HHTableConditionalUpdate(const Node *node, addr_t _obj,
                           klee::ref<klee::Expr> _key,
                           klee::ref<klee::Expr> _write_value,
                           const symbol_t &_cache_write_failed)
      : TofinoCPUModule(ModuleType::TofinoCPU_HHTableConditionalUpdate,
                        "HHTableConditionalUpdate", node),
        obj(_obj), key(_key), write_value(_write_value),
        cache_write_failed(_cache_write_failed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new HHTableConditionalUpdate(node, obj, key, write_value,
                                                  cache_write_failed);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const symbol_t &get_cache_write_failed() const { return cache_write_failed; }
};

class HHTableConditionalUpdateGenerator : public TofinoCPUModuleGenerator {
public:
  HHTableConditionalUpdateGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_HHTableConditionalUpdate,
                                 "HHTableConditionalUpdate") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::Call) {
      return std::nullopt;
    }

    const Call *dchain_allocate_new_index = static_cast<const Call *>(node);

    std::vector<const Call *> future_map_puts;
    if (!is_map_update_with_dchain(ep, dchain_allocate_new_index,
                                   future_map_puts)) {
      return std::nullopt;
    }

    if (!is_index_alloc_on_unsuccessful_map_get(ep,
                                                dchain_allocate_new_index)) {
      return std::nullopt;
    }

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index,
                                                map_objs)) {
      return std::nullopt;
    }

    if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
        !ctx.check_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable) ||
        !ctx.check_ds_impl(map_objs.vector_key,
                           DSImpl::Tofino_HeavyHitterTable)) {
      return std::nullopt;
    }

    hit_rate_t new_hh_hr = get_new_hh_probability(ep, node, map_objs.map);

    Context new_ctx = ctx;

    new_ctx.get_mutable_perf_oracle().add_controller_traffic(new_hh_hr);

    spec_impl_t spec_impl(decide(ep, node), new_ctx);

    // Get all nodes executed on a successful index allocation.
    index_alloc_check_t index_alloc_check =
        find_branch_checking_index_alloc(ep, dchain_allocate_new_index);
    assert(index_alloc_check.branch &&
           "Branch checking index allocation not found");

    spec_impl.skip.insert(index_alloc_check.branch->get_id());

    const Node *on_hh = index_alloc_check.success_on_true
                            ? index_alloc_check.branch->get_on_true()
                            : index_alloc_check.branch->get_on_false();

    on_hh->visit_nodes([&spec_impl](const Node *node) {
      spec_impl.skip.insert(node->get_id());
      return NodeVisitAction::Continue;
    });

    return spec_impl;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::Call) {
      return impls;
    }

    const Call *dchain_allocate_new_index = static_cast<const Call *>(node);

    std::vector<const Call *> future_map_puts;
    if (!is_map_update_with_dchain(ep, dchain_allocate_new_index,
                                   future_map_puts)) {
      return impls;
    }

    if (!is_index_alloc_on_unsuccessful_map_get(ep,
                                                dchain_allocate_new_index)) {
      return impls;
    }

    map_coalescing_objs_t map_objs;
    if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index,
                                                map_objs)) {
      return impls;
    }

    index_alloc_check_t index_alloc_check =
        find_branch_checking_index_alloc(ep, dchain_allocate_new_index);
    if (dchain_allocate_new_index->get_next() != index_alloc_check.branch) {
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

    klee::ref<klee::Expr> min_estimate = get_min_estimate(ep);
    ASSERT_OR_PANIC(!min_estimate.isNull(),
                    "TODO: HHTableRead not found, so we should "
                    "query the CMS for the min estimate");

    const Call *map_put = get_future_map_put(node, map_objs.map);
    ASSERT_OR_PANIC(map_put, "map_put not found");

    table_data_t table_data = get_table_data(map_put);

    const Node *next_on_hh = index_alloc_check.success_on_true
                                 ? index_alloc_check.branch->get_on_true()
                                 : index_alloc_check.branch->get_on_false();
    const Node *next_on_not_hh = index_alloc_check.success_on_true
                                     ? index_alloc_check.branch->get_on_false()
                                     : index_alloc_check.branch->get_on_true();

    Module *hh_table_update =
        new HHTableUpdate(node, table_data.obj, table_data.table_keys,
                          table_data.value, min_estimate);

    assert(false && "TODO");

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
    assert(call.function_name == "map_put");

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

  hit_rate_t get_new_hh_probability(const EP *ep, const Node *node,
                                    addr_t map) const {
    hit_rate_t node_hr = ep->get_ctx().get_profiler().get_hr(node);
    int capacity = ep->get_ctx().get_map_config(map).capacity;
    hit_rate_t churn_hr = ep->get_ctx()
                              .get_profiler()
                              .get_bdd_profile()
                              ->churn_hit_rate_top_k_flows(map, capacity);
    return node_hr * churn_hr;
  }

  const Call *get_future_map_put(const Node *node, addr_t map) const {
    for (const Call *map_put : get_future_functions(node, {"map_put"})) {
      const call_t &call = map_put->get_call();
      klee::ref<klee::Expr> map_expr = call.args.at("map").expr;

      if (expr_addr_to_obj_addr(map_expr) == map) {
        return map_put;
      }
    }

    return nullptr;
  }
};

} // namespace tofino_cpu
