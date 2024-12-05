#pragma once

#include "tofino_module.h"

namespace tofino {

class IntegerAllocatorRejuvenate : public TofinoModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> time;

public:
  IntegerAllocatorRejuvenate(const Node *node, addr_t _dchain_addr,
                             klee::ref<klee::Expr> _index,
                             klee::ref<klee::Expr> _time)
      : TofinoModule(ModuleType::Tofino_IntegerAllocatorRejuvenate,
                     "IntegerAllocatorRejuvenate", node),
        dchain_addr(_dchain_addr), index(_index), time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new IntegerAllocatorRejuvenate(node, dchain_addr, index, time);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_time() const { return time; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    // FIXME: Missing the Integer Allocator data structure.
    // return {table_id};
    return {};
  }
};

class IntegerAllocatorRejuvenateGenerator : public TofinoModuleGenerator {
public:
  IntegerAllocatorRejuvenateGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_IntegerAllocatorRejuvenate,
                              "IntegerAllocatorRejuvenate") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "dchain_rejuvenate_index") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

    if (!ctx.can_impl_ds(dchain_addr, DSImpl::Tofino_IntegerAllocator)) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(dchain_addr, DSImpl::Tofino_IntegerAllocator);

    return spec_impl_t(decide(ep, node), new_ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "dchain_rejuvenate_index") {
      return impls;
    }

    klee::ref<klee::Expr> dchain_addr_expr = call.args.at("chain").expr;
    klee::ref<klee::Expr> index = call.args.at("index").expr;
    klee::ref<klee::Expr> time = call.args.at("time").expr;

    addr_t dchain_addr = expr_addr_to_obj_addr(dchain_addr_expr);

    if (!ep->get_ctx().can_impl_ds(dchain_addr,
                                   DSImpl::Tofino_IntegerAllocator)) {
      return impls;
    }

    Module *module =
        new IntegerAllocatorRejuvenate(node, dchain_addr, index, time);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    new_ep->get_mutable_ctx().save_ds_impl(dchain_addr,
                                           DSImpl::Tofino_IntegerAllocator);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace tofino
