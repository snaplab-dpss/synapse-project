#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class CMSCountMin : public TofinoCPUModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;

public:
  CMSCountMin(const Node *node, addr_t _cms_addr, klee::ref<klee::Expr> _key,
              klee::ref<klee::Expr> _min_estimate)
      : TofinoCPUModule(ModuleType::TofinoCPU_CMSCountMin, "CMSCountMin", node),
        cms_addr(_cms_addr), key(_key), min_estimate(_min_estimate) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSCountMin(node, cms_addr, key, min_estimate);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }
};

class CMSCountMinGenerator : public TofinoCPUModuleGenerator {
public:
  CMSCountMinGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_CMSCountMin,
                                 "CMSCountMin") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "cms_count_min") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    if (!ctx.can_impl_ds(cms_addr, DSImpl::TofinoCPU_CMS)) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(cms_addr, DSImpl::TofinoCPU_CMS);

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

    if (call.function_name != "cms_count_min") {
      return impls;
    }

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;

    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    symbols_t symbols = call_node->get_locally_generated_symbols();
    symbol_t min_estimate;
    bool found = get_symbol(symbols, "min_estimate", min_estimate);
    assert(found && "Symbol min_estimate not found");

    if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::TofinoCPU_CMS)) {
      return impls;
    }

    Module *module = new CMSCountMin(node, cms_addr, key, min_estimate.expr);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    new_ep->get_mutable_ctx().save_ds_impl(cms_addr, DSImpl::TofinoCPU_CMS);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace tofino_cpu
