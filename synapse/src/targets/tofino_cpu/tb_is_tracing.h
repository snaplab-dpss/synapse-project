#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class TBIsTracing : public TofinoCPUModule {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> index_out;
  klee::ref<klee::Expr> is_tracing;

public:
  TBIsTracing(const Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _key,
              klee::ref<klee::Expr> _index_out,
              klee::ref<klee::Expr> _is_tracing)
      : TofinoCPUModule(ModuleType::TofinoCPU_TBIsTracing, "TBIsTracing", node),
        tb_addr(_tb_addr), key(_key), index_out(_index_out),
        is_tracing(_is_tracing) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TBIsTracing(node, tb_addr, key, index_out, is_tracing);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  klee::ref<klee::Expr> get_is_tracing() const { return is_tracing; }
};

class TBIsTracingGenerator : public TofinoCPUModuleGenerator {
public:
  TBIsTracingGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_TBIsTracing,
                                 "TBIsTracing") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "tb_is_tracing") {
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

    if (call.function_name != "tb_is_tracing") {
      return impls;
    }

    klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> index_out = call.args.at("index_out").out;
    klee::ref<klee::Expr> is_tracing = call.ret;

    addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

    if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::TofinoCPU_TokenBucket)) {
      return impls;
    }

    Module *module = new TBIsTracing(node, tb_addr, key, index_out, is_tracing);
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
