#pragma once

#include "x86_module.h"

namespace x86 {

class CMSTouchBuckets : public x86Module {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> time;
  symbol_t success;

public:
  CMSTouchBuckets(const Node *node, addr_t _cms_addr,
                  klee::ref<klee::Expr> _time, symbol_t _success)
      : x86Module(ModuleType::x86_CMSTouchBuckets, "CMSTouchBuckets", node),
        cms_addr(_cms_addr), time(_time), success(_success) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSTouchBuckets(node, cms_addr, time, success);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  const symbol_t &get_success() const { return success; }
};

class CMSTouchBucketsGenerator : public x86ModuleGenerator {
public:
  CMSTouchBucketsGenerator()
      : x86ModuleGenerator(ModuleType::x86_CMSTouchBuckets, "CMSTouchBuckets") {
  }

protected:
  bool bdd_node_match_pattern(const Node *node) const {
    if (node->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "cms_touch_buckets") {
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
    klee::ref<klee::Expr> time = call.args.at("time").expr;

    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::x86_CMS)) {
      return impls;
    }

    symbols_t symbols = call_node->get_locally_generated_symbols();
    symbol_t success;
    bool found = get_symbol(symbols, "success", success);
    assert(found && "Symbol success not found");

    Module *module = new CMSTouchBuckets(node, cms_addr, time, success);
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