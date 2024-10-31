#pragma once

#include "x86_module.h"

namespace x86 {

class SketchTouchBuckets : public x86Module {
private:
  addr_t sketch_addr;
  klee::ref<klee::Expr> time;
  symbol_t success;

public:
  SketchTouchBuckets(const Node *node, addr_t _sketch_addr,
                     klee::ref<klee::Expr> _time, symbol_t _success)
      : x86Module(ModuleType::x86_SketchTouchBuckets, "SketchTouchBuckets",
                  node),
        sketch_addr(_sketch_addr), time(_time), success(_success) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new SketchTouchBuckets(node, sketch_addr, time, success);
    return cloned;
  }

  addr_t get_sketch_addr() const { return sketch_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  const symbol_t &get_success() const { return success; }
};

class SketchTouchBucketsGenerator : public x86ModuleGenerator {
public:
  SketchTouchBucketsGenerator()
      : x86ModuleGenerator(ModuleType::x86_SketchTouchBuckets,
                           "SketchTouchBuckets") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "sketch_touch_buckets") {
      return false;
    }

    return true;
  }

  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (!bdd_node_match_pattern(node)) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);

    if (!can_impl_ds(ep, call_node, "sketch", DSImpl::x86_Sketch)) {
      return std::nullopt;
    }

    return spec_impl_t(decide(ep, node), ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (!bdd_node_match_pattern(node)) {
      return impls;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (!can_impl_ds(ep, call_node, "sketch", DSImpl::x86_Sketch)) {
      return impls;
    }

    klee::ref<klee::Expr> sketch_addr_expr = call.args.at("sketch").expr;
    klee::ref<klee::Expr> time = call.args.at("time").expr;

    addr_t sketch_addr = expr_addr_to_obj_addr(sketch_addr_expr);

    symbols_t symbols = call_node->get_locally_generated_symbols();
    symbol_t success;
    bool found = get_symbol(symbols, "success", success);
    assert(found && "Symbol success not found");

    Module *module = new SketchTouchBuckets(node, sketch_addr, time, success);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(sketch_addr, DSImpl::x86_Sketch);

    return impls;
  }
};

} // namespace x86