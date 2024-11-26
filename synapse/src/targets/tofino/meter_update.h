#pragma once

#include "tofino_module.h"

namespace tofino {

class MeterUpdate : public TofinoModule {
private:
  DS_ID table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> hit;
  klee::ref<klee::Expr> pass;

public:
  MeterUpdate(const Node *node, DS_ID _table_id, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _pkt_len, klee::ref<klee::Expr> _hit,
              klee::ref<klee::Expr> _pass)
      : TofinoModule(ModuleType::Tofino_MeterUpdate, "MeterUpdate", node),
        table_id(_table_id), obj(_obj), keys(_keys), pkt_len(_pkt_len),
        hit(_hit), pass(_pass) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new MeterUpdate(node, table_id, obj, keys, pkt_len, hit, pass);
    return cloned;
  }

  DS_ID get_table_id() const { return table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_pkt_len() const { return pkt_len; }
  klee::ref<klee::Expr> get_hit() const { return hit; }
  klee::ref<klee::Expr> get_pass() const { return pass; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {table_id};
  }
};

class MeterUpdateGenerator : public TofinoModuleGenerator {
public:
  MeterUpdateGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_MeterUpdate, "MeterUpdate") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *tb_is_tracing = static_cast<const Call *>(node);
    const call_t &call = tb_is_tracing->get_call();

    const Call *tb_update_and_check;
    if (!is_tb_tracing_check_followed_by_update_on_true(tb_is_tracing,
                                                        tb_update_and_check)) {
      return std::nullopt;
    }

    addr_t obj;
    tb_config_t cfg;
    std::vector<klee::ref<klee::Expr>> keys;
    klee::ref<klee::Expr> pkt_len;
    klee::ref<klee::Expr> hit;
    klee::ref<klee::Expr> pass;
    DS_ID id;

    if (!get_tb_data(ep, tb_is_tracing, tb_update_and_check, obj, cfg, keys,
                     pkt_len, hit, pass, id)) {
      return std::nullopt;
    }

    if (!ctx.can_impl_ds(obj, DSImpl::Tofino_Table)) {
      return std::nullopt;
    }

    std::unordered_set<DS_ID> deps;
    Meter *meter = build_meter(ep, node, id, cfg, keys, deps);

    if (!meter) {
      return std::nullopt;
    }

    delete meter;

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(obj, DSImpl::Tofino_Meter);

    spec_impl_t spec_impl(decide(ep, node), new_ctx);
    spec_impl.skip.insert(tb_update_and_check->get_id());

    return spec_impl;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *tb_is_tracing = static_cast<const Call *>(node);
    const call_t &call = tb_is_tracing->get_call();

    const Call *tb_update_and_check;
    if (!is_tb_tracing_check_followed_by_update_on_true(tb_is_tracing,
                                                        tb_update_and_check)) {
      return impls;
    }

    addr_t obj;
    tb_config_t cfg;
    std::vector<klee::ref<klee::Expr>> keys;
    klee::ref<klee::Expr> pkt_len;
    klee::ref<klee::Expr> hit;
    klee::ref<klee::Expr> pass;
    DS_ID id;

    if (!get_tb_data(ep, tb_is_tracing, tb_update_and_check, obj, cfg, keys,
                     pkt_len, hit, pass, id)) {
      return impls;
    }

    if (!ep->get_ctx().can_impl_ds(obj, DSImpl::Tofino_Meter)) {
      return impls;
    }

    std::unordered_set<DS_ID> deps;
    Meter *meter = build_meter(ep, node, id, cfg, keys, deps);

    if (!meter) {
      return impls;
    }

    Module *module = new MeterUpdate(node, id, obj, keys, pkt_len, hit, pass);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    const Node *new_next;
    BDD *bdd =
        delete_future_tb_update(new_ep, node, tb_update_and_check, new_next);

    place_meter(new_ep, obj, meter, deps);

    EPLeaf leaf(ep_node, new_next);
    new_ep->process_leaf(ep_node, {leaf});
    new_ep->replace_bdd(bdd);

    return impls;
  }

  bool get_tb_data(const EP *ep, const Call *tb_is_tracing,
                   const Call *tb_update_and_check, addr_t &obj,
                   tb_config_t &cfg, std::vector<klee::ref<klee::Expr>> &keys,
                   klee::ref<klee::Expr> pkt_len, klee::ref<klee::Expr> &hit,
                   klee::ref<klee::Expr> pass, DS_ID &id) const {
    const call_t &call_is_tracing = tb_is_tracing->get_call();
    assert(call_is_tracing.function_name == "tb_is_tracing");

    const call_t &call_update = tb_update_and_check->get_call();
    assert(call_update.function_name == "tb_update_and_check");

    klee::ref<klee::Expr> tb_addr_expr = call_is_tracing.args.at("tb").expr;
    klee::ref<klee::Expr> key = call_is_tracing.args.at("key").in;
    klee::ref<klee::Expr> index_out = call_is_tracing.args.at("index_out").out;
    klee::ref<klee::Expr> is_tracing = call_is_tracing.ret;

    obj = expr_addr_to_obj_addr(tb_addr_expr);
    keys = Table::build_keys(key);
    pkt_len = call_update.args.at("pkt_len").expr;
    hit = is_tracing;
    pass = call_update.ret;
    id = "tb_" + std::to_string(tb_is_tracing->get_id());

    const Context &ctx = ep->get_ctx();
    cfg = ctx.get_tb_config(obj);

    return true;
  }

  Meter *build_meter(const EP *ep, const Node *node, DS_ID id,
                     const tb_config_t &cfg,
                     const std::vector<klee::ref<klee::Expr>> &keys,
                     std::unordered_set<DS_ID> &deps) const {
    std::vector<bits_t> keys_size;
    for (klee::ref<klee::Expr> key : keys) {
      keys_size.push_back(key->getWidth());
    }

    Meter *meter = new Meter(id, cfg.capacity, cfg.rate, cfg.burst, keys_size);

    const TofinoContext *tofino_ctx = get_tofino_ctx(ep);
    deps = tofino_ctx->get_stateful_deps(ep, node);

    if (!tofino_ctx->check_placement(ep, meter, deps)) {
      delete meter;
      return nullptr;
    }

    return meter;
  }

  void place_meter(EP *ep, addr_t obj, Meter *meter,
                   const std::unordered_set<DS_ID> &deps) const {
    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(ep);
    ep->get_mutable_ctx().save_ds_impl(obj, DSImpl::Tofino_Meter);
    tofino_ctx->place(ep, obj, meter, deps);
  }

  BDD *delete_future_tb_update(EP *ep, const Node *node,
                               const Call *tb_update_and_check,
                               const Node *&new_next) const {
    const BDD *old_bdd = ep->get_bdd();
    BDD *new_bdd = new BDD(*old_bdd);

    const Node *next = node->get_next();

    if (next) {
      new_next = new_bdd->get_node_by_id(next->get_id());
    } else {
      new_next = nullptr;
    }

    bool replace_next = (tb_update_and_check == next);
    Node *replacement = delete_non_branch_node_from_bdd(
        ep, new_bdd, tb_update_and_check->get_id());

    if (replace_next) {
      new_next = replacement;
    }

    return new_bdd;
  }
};

} // namespace tofino
