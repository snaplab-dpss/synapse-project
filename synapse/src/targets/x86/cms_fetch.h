#pragma once

#include "x86_module.h"

namespace x86 {

class CMSFetch : public x86Module {
private:
  addr_t cms_addr;
  symbol_t overflow;

public:
  CMSFetch(const Node *node, addr_t _cms_addr, symbol_t _overflow)
      : x86Module(ModuleType::x86_CMSFetch, "CMSFetch", node),
        cms_addr(_cms_addr), overflow(_overflow) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSFetch(node, cms_addr, overflow);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  const symbol_t &get_overflow() const { return overflow; }
};

class CMSFetchGenerator : public x86ModuleGenerator {
public:
  CMSFetchGenerator()
      : x86ModuleGenerator(ModuleType::x86_CMSFetch, "CMSFetch") {}

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "cms_fetch") {
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

    if (!ctx.can_impl_ds(cms_addr, DSImpl::x86_CMS)) {
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

    if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::x86_CMS)) {
      return impls;
    }

    symbols_t symbols = call_node->get_locally_generated_symbols();
    symbol_t overflow;
    bool found = get_symbol(symbols, "overflow", overflow);
    assert(found && "Symbol overflow not found");

    Module *module = new CMSFetch(node, cms_addr, overflow);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    new_ep->get_mutable_ctx().save_ds_impl(cms_addr, DSImpl::x86_CMS);

    return impls;
  }
};

} // namespace x86
