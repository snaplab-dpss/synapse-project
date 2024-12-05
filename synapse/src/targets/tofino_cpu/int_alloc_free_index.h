#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class IntegerAllocatorFreeIndex : public TofinoCPUModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;

public:
  IntegerAllocatorFreeIndex(const Node *node, addr_t _dchain_addr,
                            klee::ref<klee::Expr> _index)
      : TofinoCPUModule(ModuleType::TofinoCPU_IntegerAllocatorFreeIndex,
                        "IntegerAllocatorFreeIndex", node),
        dchain_addr(_dchain_addr), index(_index) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new IntegerAllocatorFreeIndex(node, dchain_addr, index);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
};

class IntegerAllocatorFreeIndexGenerator : public TofinoCPUModuleGenerator {
public:
  IntegerAllocatorFreeIndexGenerator()
      : TofinoCPUModuleGenerator(
            ModuleType::TofinoCPU_IntegerAllocatorFreeIndex,
            "IntegerAllocatorFreeIndex") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "dchain_free_index") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

    if (!ctx.check_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
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

    if (call.function_name != "dchain_free_index") {
      return impls;
    }

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;

    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

    if (!ep->get_ctx().check_ds_impl(dchain_addr,
                                     DSImpl::Tofino_IntegerAllocator)) {
      return impls;
    }

    Module *module = new IntegerAllocatorFreeIndex(node, dchain_addr, index);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace tofino_cpu
