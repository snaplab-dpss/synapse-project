#include "hh_table_conditional_update.h"

#include "if.h"
#include "then.h"
#include "else.h"
#include "../tofino/hh_table_read.h"
#include "hh_table_read.h"
#include "hh_table_update.h"

namespace tofino_cpu {

using tofino::HHTable;
using tofino::Table;

namespace {
struct table_data_t {
  addr_t obj;
  klee::ref<klee::Expr> key;
  std::vector<klee::ref<klee::Expr>> table_keys;
  klee::ref<klee::Expr> value;

  table_data_t(const Call *map_put) {
    const call_t &call = map_put->get_call();
    ASSERT(call.function_name == "map_put", "Not a map_put call");

    obj = expr_addr_to_obj_addr(call.args.at("map").expr);
    key = call.args.at("key").in;
    table_keys = Table::build_keys(key);
    value = call.args.at("value").expr;
  }
};

klee::ref<klee::Expr> get_min_estimate(const EP *ep) {
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

hit_rate_t get_new_hh_probability(const EP *ep, const Node *node, addr_t map) {
  hit_rate_t node_hr = ep->get_ctx().get_profiler().get_hr(node);
  int capacity = ep->get_ctx().get_map_config(map).capacity;
  hit_rate_t churn_hr = ep->get_ctx()
                            .get_profiler()
                            .get_bdd_profile()
                            ->churn_hit_rate_top_k_flows(map, capacity);
  return node_hr * churn_hr;
}

const Call *get_future_map_put(const Node *node, addr_t map) {
  for (const Call *map_put : get_future_functions(node, {"map_put"})) {
    const call_t &call = map_put->get_call();
    klee::ref<klee::Expr> map_expr = call.args.at("map").expr;

    if (expr_addr_to_obj_addr(map_expr) == map) {
      return map_put;
    }
  }

  return nullptr;
}
} // namespace

std::optional<spec_impl_t>
HHTableConditionalUpdateGenerator::speculate(const EP *ep, const Node *node,
                                             const Context &ctx) const {
  if (node->get_type() != NodeType::Call) {
    return std::nullopt;
  }

  const Call *dchain_allocate_new_index = static_cast<const Call *>(node);

  std::vector<const Call *> future_map_puts;
  if (!is_map_update_with_dchain(ep, dchain_allocate_new_index,
                                 future_map_puts)) {
    return std::nullopt;
  }

  if (!is_index_alloc_on_unsuccessful_map_get(ep, dchain_allocate_new_index)) {
    return std::nullopt;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index,
                                              map_objs)) {
    return std::nullopt;
  }

  if (!ctx.check_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
      !ctx.check_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable)) {
    return std::nullopt;
  }

  hit_rate_t new_hh_hr = get_new_hh_probability(ep, node, map_objs.map);

  Context new_ctx = ctx;

  new_ctx.get_mutable_perf_oracle().add_controller_traffic(new_hh_hr);

  spec_impl_t spec_impl(decide(ep, node), new_ctx);

  // Get all nodes executed on a successful index allocation.
  branch_direction_t index_alloc_check =
      find_branch_checking_index_alloc(ep, dchain_allocate_new_index);
  ASSERT(index_alloc_check.branch,
         "Branch checking index allocation not found");

  spec_impl.skip.insert(index_alloc_check.branch->get_id());

  const Node *on_hh = index_alloc_check.direction
                          ? index_alloc_check.branch->get_on_true()
                          : index_alloc_check.branch->get_on_false();

  on_hh->visit_nodes([&spec_impl](const Node *node) {
    spec_impl.skip.insert(node->get_id());
    return NodeVisitAction::Continue;
  });

  return spec_impl;
}

std::vector<impl_t>
HHTableConditionalUpdateGenerator::process_node(const EP *ep,
                                                const Node *node) const {
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

  if (!is_index_alloc_on_unsuccessful_map_get(ep, dchain_allocate_new_index)) {
    return impls;
  }

  map_coalescing_objs_t map_objs;
  if (!get_map_coalescing_objs_from_dchain_op(ep, dchain_allocate_new_index,
                                              map_objs)) {
    return impls;
  }

  branch_direction_t index_alloc_check =
      find_branch_checking_index_alloc(ep, dchain_allocate_new_index);
  if (dchain_allocate_new_index->get_next() != index_alloc_check.branch) {
    return impls;
  }

  if (!ep->get_ctx().check_ds_impl(map_objs.map,
                                   DSImpl::Tofino_HeavyHitterTable) ||
      !ep->get_ctx().check_ds_impl(map_objs.dchain,
                                   DSImpl::Tofino_HeavyHitterTable)) {
    return impls;
  }

  klee::ref<klee::Expr> min_estimate = get_min_estimate(ep);
  ASSERT(!min_estimate.isNull(), "TODO: HHTableRead not found, so we should "
                                 "query the CMS for the min estimate");

  const Call *map_put = get_future_map_put(node, map_objs.map);
  ASSERT(map_put, "map_put not found");

  table_data_t table_data(map_put);

  const Node *next_on_hh = index_alloc_check.direction
                               ? index_alloc_check.branch->get_on_true()
                               : index_alloc_check.branch->get_on_false();
  const Node *next_on_not_hh = index_alloc_check.direction
                                   ? index_alloc_check.branch->get_on_false()
                                   : index_alloc_check.branch->get_on_true();

  Module *hh_table_update =
      new HHTableUpdate(node, table_data.obj, table_data.table_keys,
                        table_data.value, min_estimate);

  ASSERT(false, "TODO");

  return impls;
}

} // namespace tofino_cpu
