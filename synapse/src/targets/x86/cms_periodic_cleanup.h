#pragma once

#include "x86_module.h"

namespace x86 {

class CMSPeriodicCleanup : public x86Module {
private:
  addr_t cms_addr;

public:
  CMSPeriodicCleanup(const Node *node, addr_t _cms_addr)
      : x86Module(ModuleType::x86_CMSPeriodicCleanup, "CMSPeriodicCleanup",
                  node),
        cms_addr(_cms_addr) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSPeriodicCleanup(node, cms_addr);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
};

class CMSPeriodicCleanupGenerator : public x86ModuleGenerator {
public:
  CMSPeriodicCleanupGenerator()
      : x86ModuleGenerator(ModuleType::x86_CMSPeriodicCleanup,
                           "CMSPeriodicCleanup") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "cms_periodic_cleanup") {
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

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    if (!ctx.can_impl_ds(cms_addr, DSImpl::x86_CountMinSketch)) {
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

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;

    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::x86_CountMinSketch)) {
      return impls;
    }

    Module *module = new CMSPeriodicCleanup(node, cms_addr);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    new_ep->get_mutable_ctx().save_ds_impl(cms_addr,
                                           DSImpl::x86_CountMinSketch);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace x86
