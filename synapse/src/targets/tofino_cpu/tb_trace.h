#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class TBTrace : public TofinoCPUModule {
private:
  addr_t tb_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> pkt_len;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> index_out;
  klee::ref<klee::Expr> successfuly_tracing;

public:
  TBTrace(const Node *node, addr_t _tb_addr, klee::ref<klee::Expr> _key,
          klee::ref<klee::Expr> _pkt_len, klee::ref<klee::Expr> _time,
          klee::ref<klee::Expr> _index_out, klee::ref<klee::Expr> _is_tracing)
      : TofinoCPUModule(ModuleType::TofinoCPU_TBTrace, "TBTrace", node),
        tb_addr(_tb_addr), key(_key), pkt_len(_pkt_len), time(_time),
        index_out(_index_out), successfuly_tracing(_is_tracing) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TBTrace(node, tb_addr, key, pkt_len, time, index_out,
                                 successfuly_tracing);
    return cloned;
  }

  addr_t get_tb_addr() const { return tb_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_pkt_len() const { return pkt_len; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  klee::ref<klee::Expr> get_successfuly_tracing() const {
    return successfuly_tracing;
  }
};

class TBTraceGenerator : public TofinoCPUModuleGenerator {
public:
  TBTraceGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_TBTrace, "TBTrace") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "tb_trace") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
    addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

    if (!ctx.can_impl_ds(tb_addr, DSImpl::TofinoCPU_TB)) {
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

    if (call.function_name != "tb_trace") {
      return impls;
    }

    klee::ref<klee::Expr> tb_addr_expr = call.args.at("tb").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;
    klee::ref<klee::Expr> pkt_len = call.args.at("pkt_len").expr;
    klee::ref<klee::Expr> time = call.args.at("time").expr;
    klee::ref<klee::Expr> index_out = call.args.at("index_out").out;
    klee::ref<klee::Expr> successfuly_tracing = call.ret;

    addr_t tb_addr = expr_addr_to_obj_addr(tb_addr_expr);

    if (!ep->get_ctx().can_impl_ds(tb_addr, DSImpl::TofinoCPU_TB)) {
      return impls;
    }

    Module *module = new TBTrace(node, tb_addr, key, pkt_len, time, index_out,
                                 successfuly_tracing);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(tb_addr, DSImpl::TofinoCPU_TB);

    return impls;
  }
};

} // namespace tofino_cpu
