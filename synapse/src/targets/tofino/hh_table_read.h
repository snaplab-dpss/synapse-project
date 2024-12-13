#pragma once

#include "tofino_module.h"

#define CMS_WIDTH_PARAM "cms_width"
#define CMS_HEIGHT_PARAM "cms_height"
#define CMS_THRESHOLD_PARAM "cms_threshold"

#define CMS_WIDTH 1024
#define CMS_HEIGHT 4
#define CMS_THRESHOLD 1_000_000

namespace tofino {

class HHTableRead : public TofinoModule {
private:
  DS_ID hh_table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> map_has_this_key;
  klee::ref<klee::Expr> min_estimate;

public:
  HHTableRead(const Node *node, DS_ID _hh_table_id, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _value,
              klee::ref<klee::Expr> _map_has_this_key,
              klee::ref<klee::Expr> _min_estimate)
      : TofinoModule(ModuleType::Tofino_HHTableRead, "HHTableRead", node),
        hh_table_id(_hh_table_id), obj(_obj), keys(_keys), value(_value),
        map_has_this_key(_map_has_this_key), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new HHTableRead(node, hh_table_id, obj, keys, value,
                                     map_has_this_key, min_estimate);
    return cloned;
  }

  DS_ID get_hh_table_id() const { return hh_table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_hit() const { return map_has_this_key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {hh_table_id};
  }
};

class HHTableReadGenerator : public TofinoModuleGenerator {
public:
  HHTableReadGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_HHTableRead, "HHTableRead") {}

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

    if (!ctx.can_impl_ds(map_objs.map, DSImpl::Tofino_HeavyHitterTable) ||
        !ctx.can_impl_ds(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable)) {
      return std::nullopt;
    }

    branch_direction_t mpsc = get_map_get_success_check(map_get);
    if (!mpsc.branch) {
      return std::nullopt;
    }

    table_data_t table_data = get_table_data(ep, map_get);

    if (!can_build_or_reuse_hh_table(
            ep, node, table_data.obj, table_data.table_keys,
            table_data.num_entries, CMS_WIDTH, CMS_HEIGHT)) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(map_objs.map, DSImpl::Tofino_HeavyHitterTable);
    new_ctx.save_ds_impl(map_objs.dchain, DSImpl::Tofino_HeavyHitterTable);

    update_map_get_success_hit_rate(new_ctx, map_get, table_data.key,
                                    table_data.num_entries, mpsc);

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

    if (!ep->get_ctx().can_impl_ds(map_objs.map,
                                   DSImpl::Tofino_HeavyHitterTable) ||
        !ep->get_ctx().can_impl_ds(map_objs.dchain,
                                   DSImpl::Tofino_HeavyHitterTable)) {
      return impls;
    }

    branch_direction_t mpsc = get_map_get_success_check(map_get);
    if (!mpsc.branch) {
      return impls;
    }

    table_data_t table_data = get_table_data(ep, map_get);

    HHTable *hh_table =
        build_or_reuse_hh_table(ep, node, table_data.obj, table_data.table_keys,
                                table_data.num_entries, CMS_WIDTH, CMS_HEIGHT);

    if (!hh_table) {
      return impls;
    }

    Module *module =
        new HHTableRead(node, hh_table->id, table_data.obj,
                        table_data.table_keys, table_data.read_value,
                        table_data.map_has_this_key, table_data.min_estimate);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    new_ep->get_mutable_ctx().save_ds_impl(map_objs.map,
                                           DSImpl::Tofino_HeavyHitterTable);
    new_ep->get_mutable_ctx().save_ds_impl(map_objs.dchain,
                                           DSImpl::Tofino_HeavyHitterTable);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->place(new_ep, node, map_objs.map, hh_table);

    update_map_get_success_hit_rate(new_ep->get_mutable_ctx(), map_get,
                                    table_data.key, table_data.num_entries,
                                    mpsc);

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
    u32 num_entries;
  };

  table_data_t get_table_data(const EP *ep, const Call *map_get) const {
    const call_t &call = map_get->get_call();
    ASSERT(call.function_name == "map_get", "Not a map_get call");

    symbols_t symbols = map_get->get_locally_generated_symbols();

    klee::ref<klee::Expr> obj_expr = call.args.at("map").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> value_out = call.args.at("value_out").out;

    symbol_t map_has_this_key;
    bool found = get_symbol(symbols, "map_has_this_key", map_has_this_key);
    ASSERT(found, "Symbol map_has_this_key not found");

    addr_t obj = expr_addr_to_obj_addr(obj_expr);
    klee::ref<klee::Expr> min_estimate = solver_toolbox.create_new_symbol(
        "min_estimate_" + std::to_string(map_get->get_id()), 32);
    u32 capacity = ep->get_ctx().get_map_config(obj).capacity;

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

  void update_map_get_success_hit_rate(Context &ctx, const Node *map_get,
                                       klee::ref<klee::Expr> key, u32 capacity,
                                       const branch_direction_t &mgsc) const {
    hit_rate_t success_rate =
        get_hh_table_hit_success_rate(ctx, map_get, key, capacity);

    ASSERT(mgsc.branch, "No branch checking map_get success");
    const Node *on_success = mgsc.direction ? mgsc.branch->get_on_true()
                                            : mgsc.branch->get_on_false();
    const Node *on_failure = mgsc.direction ? mgsc.branch->get_on_false()
                                            : mgsc.branch->get_on_true();

    hit_rate_t branch_hr = ctx.get_profiler().get_hr(mgsc.branch);

    ctx.get_mutable_profiler().set(on_success->get_ordered_branch_constraints(),
                                   branch_hr * success_rate);
    ctx.get_mutable_profiler().set(on_failure->get_ordered_branch_constraints(),
                                   branch_hr * (1 - success_rate));
  }
};

} // namespace tofino
