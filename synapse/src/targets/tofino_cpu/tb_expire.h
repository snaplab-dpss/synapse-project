#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class TBExpire : public TofinoCPUModule {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> time;

public:
  TBExpire(const Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _time)
      : TofinoCPUModule(ModuleType::TofinoCPU_TBExpire, "TBExpire", node),
        tb_addr(_tb_addr), time(_time) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TBExpire(node, tb_addr, time);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
};

class TBExpireGenerator : public TofinoCPUModuleGenerator {
public:
  TBExpireGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_TBExpire, "TBExpire") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "tb_expire") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
    addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

    if (!ctx.can_impl_ds(tb_addr, DSImpl::TofinoCPU_TokenBucket)) {
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
    const call_t &call = call_node->get_call();

    if (call.function_name != "tb_expire") {
      return impls;
    }

    klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
    klee::ref<klee::Expr> time = call.args.at("time").expr;

    addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

    if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::TofinoCPU_TokenBucket)) {
      return impls;
    }

    Module *module = new TBExpire(node, tb_addr, time);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(tb_addr,
                                           DSImpl::TofinoCPU_TokenBucket);

    return impls;
  }
};

} // namespace tofino_cpu
