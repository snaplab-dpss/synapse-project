#pragma once

#include "x86_module.h"

namespace x86 {

class DchainFreeIndex : public x86Module {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;

public:
  DchainFreeIndex(const Node *node, addr_t _dchain_addr,
                  klee::ref<klee::Expr> _index)
      : x86Module(ModuleType::x86_DchainFreeIndex, "DchainFreeIndex", node),
        dchain_addr(_dchain_addr), index(_index) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new DchainFreeIndex(node, dchain_addr, index);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
};

class DchainFreeIndexGenerator : public x86ModuleGenerator {
public:
  DchainFreeIndexGenerator()
      : x86ModuleGenerator(ModuleType::x86_DchainFreeIndex, "DchainFreeIndex") {
  }

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "dchain_free_index") {
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
    const call_t &call = call_node->get_call();

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

    if (!ctx.can_impl_ds(dchain_addr, DSImpl::x86_DoubleChain)) {
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

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;

    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

    if (!ep->get_ctx().can_impl_ds(dchain_addr, DSImpl::x86_DoubleChain)) {
      return impls;
    }

    Module *module = new DchainFreeIndex(node, dchain_addr, index);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(dchain_addr,
                                           DSImpl::x86_DoubleChain);

    return impls;
  }
};

} // namespace x86
