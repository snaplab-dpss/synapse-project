#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class MeterInsert : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> success;

public:
  MeterInsert(const Node *node, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _success)
      : TofinoCPUModule(ModuleType::TofinoCPU_MeterInsert, "MeterInsert", node),
        obj(_obj), keys(_keys), success(_success) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    MeterInsert *cloned = new MeterInsert(node, obj, keys, success);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_success() const { return success; }

  tofino::Meter *get_meter(const EP *ep) const {
    const Context &ctx = ep->get_ctx();
    const tofino::TofinoContext *tofino_ctx =
        ctx.get_target_ctx<tofino::TofinoContext>();
    const std::unordered_set<tofino::DS *> &data_structures =
        tofino_ctx->get_ds(obj);
    assert(data_structures.size() == 1);
    assert((*data_structures.begin())->type == tofino::DSType::METER);
    return static_cast<tofino::Meter *>(*data_structures.begin());
  }
};

class MeterInsertGenerator : public TofinoCPUModuleGenerator {
public:
  MeterInsertGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_MeterInsert,
                                 "MeterInsert") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *tb_trace = static_cast<const Call *>(node);
    const call_t &call = tb_trace->get_call();

    if (call.function_name != "tb_trace") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
    addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

    if (!ctx.can_impl_ds(tb_addr, DSImpl::Tofino_Meter)) {
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

    const Call *tb_trace = static_cast<const Call *>(node);
    const call_t &call = tb_trace->get_call();

    if (call.function_name != "tb_trace") {
      return impls;
    }

    addr_t obj;
    std::vector<klee::ref<klee::Expr>> keys;
    klee::ref<klee::Expr> success;
    get_tb_data(tb_trace, obj, keys, success);

    if (!ep->get_ctx().check_ds_impl(obj, DSImpl::Tofino_Meter)) {
      return impls;
    }

    Module *module = new MeterInsert(node, obj, keys, success);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  void get_tb_data(const Call *tb_trace, addr_t &obj,
                   std::vector<klee::ref<klee::Expr>> &keys,
                   klee::ref<klee::Expr> &success) const {
    const call_t &call = tb_trace->get_call();

    klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> successfuly_tracing = call.ret;

    obj = expr_addr_to_obj_addr(tb_addr_expr);
    keys = Table::build_keys(key);
    success = successfuly_tracing;
  }
};

} // namespace tofino_cpu
