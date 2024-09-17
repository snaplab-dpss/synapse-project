#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class SketchComputeHashes : public TofinoCPUModule {
private:
  addr_t sketch_addr;
  klee::ref<klee::Expr> key;

public:
  SketchComputeHashes(const Node *node, addr_t _sketch_addr,
                      klee::ref<klee::Expr> _key)
      : TofinoCPUModule(ModuleType::TofinoCPU_SketchComputeHashes,
                        "SketchComputeHashes", node),
        sketch_addr(_sketch_addr), key(_key) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new SketchComputeHashes(node, sketch_addr, key);
    return cloned;
  }

  addr_t get_sketch_addr() const { return sketch_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class SketchComputeHashesGenerator : public TofinoCPUModuleGenerator {
public:
  SketchComputeHashesGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_SketchComputeHashes,
                                 "SketchComputeHashes") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "sketch_compute_hashes") {
      return std::nullopt;
    }

    if (!can_place(ep, call_node, "sketch",
                   PlacementDecision::TofinoCPU_Sketch)) {
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

    if (call.function_name != "sketch_compute_hashes") {
      return impls;
    }

    if (!can_place(ep, call_node, "sketch",
                   PlacementDecision::TofinoCPU_Sketch)) {
      return impls;
    }

    klee::ref<klee::Expr> sketch_addr_expr = call.args.at("sketch").expr;
    klee::ref<klee::Expr> key = call.args.at("key").expr;

    addr_t sketch_addr = expr_addr_to_obj_addr(sketch_addr_expr);

    Module *module = new SketchComputeHashes(node, sketch_addr, key);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    place(new_ep, sketch_addr, PlacementDecision::TofinoCPU_Sketch);

    return impls;
  }
};

} // namespace tofino_cpu
