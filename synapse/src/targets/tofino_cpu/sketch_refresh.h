#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class SketchRefresh : public TofinoCPUModule {
private:
  addr_t sketch_addr;
  klee::ref<klee::Expr> time;

public:
  SketchRefresh(const Node *node, addr_t _sketch_addr,
                klee::ref<klee::Expr> _time)
      : TofinoCPUModule(ModuleType::TofinoCPU_SketchRefresh, "SketchRefresh",
                        node),
        sketch_addr(_sketch_addr), time(_time) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new SketchRefresh(node, sketch_addr, time);
    return cloned;
  }

  addr_t get_sketch_addr() const { return sketch_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
};

class SketchRefreshGenerator : public TofinoCPUModuleGenerator {
public:
  SketchRefreshGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_SketchRefresh,
                                 "SketchRefresh") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "sketch_refresh") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> sketch_addr_expr = call.args.at("sketch").expr;
    addr_t sketch_addr = expr_addr_to_obj_addr(sketch_addr_expr);

    if (!ctx.can_impl_ds(sketch_addr, DSImpl::TofinoCPU_Sketch)) {
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

    if (call.function_name != "sketch_refresh") {
      return impls;
    }

    klee::ref<klee::Expr> sketch_addr_expr = call.args.at("sketch").expr;
    klee::ref<klee::Expr> time = call.args.at("time").expr;

    addr_t sketch_addr = expr_addr_to_obj_addr(sketch_addr_expr);

    if (!ep->get_ctx().can_impl_ds(sketch_addr, DSImpl::TofinoCPU_Sketch)) {
      return impls;
    }

    Module *module = new SketchRefresh(node, sketch_addr, time);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(sketch_addr,
                                           DSImpl::TofinoCPU_Sketch);

    return impls;
  }
};

} // namespace tofino_cpu
