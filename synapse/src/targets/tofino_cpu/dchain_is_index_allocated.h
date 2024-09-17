#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class DchainIsIndexAllocated : public TofinoCPUModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;
  symbol_t is_allocated;

public:
  DchainIsIndexAllocated(const Node *node, addr_t _dchain_addr,
                         klee::ref<klee::Expr> _index,
                         const symbol_t &_is_allocated)
      : TofinoCPUModule(ModuleType::TofinoCPU_DchainIsIndexAllocated,
                        "DchainIsIndexAllocated", node),
        dchain_addr(_dchain_addr), index(_index), is_allocated(_is_allocated) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new DchainIsIndexAllocated(node, dchain_addr, index, is_allocated);
    return cloned;
  }

  const addr_t &get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  const symbol_t &get_is_allocated() const { return is_allocated; }
};

class DchainIsIndexAllocatedGenerator : public TofinoCPUModuleGenerator {
public:
  DchainIsIndexAllocatedGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_DchainIsIndexAllocated,
                                 "DchainIsIndexAllocated") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "dchain_is_index_allocated") {
      return std::nullopt;
    }

    if (!can_place(ep, call_node, "chain",
                   PlacementDecision::TofinoCPU_Dchain)) {
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

    if (call.function_name != "dchain_is_index_allocated") {
      return impls;
    }

    if (!can_place(ep, call_node, "chain",
                   PlacementDecision::TofinoCPU_Dchain)) {
      return impls;
    }

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;

    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);
    symbols_t symbols = call_node->get_locally_generated_symbols();
    symbol_t is_allocated;
    bool found = get_symbol(symbols, "dchain_is_index_allocated", is_allocated);
    assert(found && "Symbol dchain_is_index_allocated not found");

    Module *module =
        new DchainIsIndexAllocated(node, dchain_addr, index, is_allocated);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    place(new_ep, dchain_addr, PlacementDecision::TofinoCPU_Dchain);

    return impls;
  }
};

} // namespace tofino_cpu
