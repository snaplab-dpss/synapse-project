#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class CMSUpdate : public TofinoCPUModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;

public:
  CMSUpdate(const Node *node, addr_t _cms_addr, klee::ref<klee::Expr> _key)
      : TofinoCPUModule(ModuleType::TofinoCPU_CMSUpdate, "CMSUpdate", node),
        cms_addr(_cms_addr), key(_key) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new CMSUpdate(node, cms_addr, key);
    return cloned;
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
};

class CMSUpdateGenerator : public TofinoCPUModuleGenerator {
public:
  CMSUpdateGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_CMSUpdate, "CMSUpdate") {
  }

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "cms_increment") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    if (!ctx.check_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch)) {
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

    if (call.function_name != "cms_increment") {
      return impls;
    }

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;

    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    if (!ep->get_ctx().check_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch)) {
      return impls;
    }

    Module *module = new CMSUpdate(node, cms_addr, key);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace tofino_cpu
