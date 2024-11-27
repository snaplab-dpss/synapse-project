#pragma once

#include "tofino_module.h"

namespace tofino {

class CMSIncrement : public TofinoModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;

public:
  CMSIncrement(const Node *node, addr_t _cms_addr, klee::ref<klee::Expr> _key)
      : TofinoModule(ModuleType::Tofino_CMSIncrement, "CMSIncrement", node),
        cms_addr(_cms_addr), key(_key) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    return new CMSIncrement(node, cms_addr, key);
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    // FIXME:
    return {};
  }
};

class CMSIncrementGenerator : public TofinoModuleGenerator {
public:
  CMSIncrementGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_CMSIncrement, "CMSIncrement") {
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

    if (!ctx.can_impl_ds(cms_addr, DSImpl::Tofino_CountMinSketch)) {
      return std::nullopt;
    }

    const cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_addr);
    if (!can_build_or_reuse_cms(ep, node, cms_addr, cfg.width, cfg.height)) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch);

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

    if (call.function_name != "cms_increment") {
      return impls;
    }

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;

    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::Tofino_CountMinSketch)) {
      return impls;
    }

    const cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_addr);
    CountMinSketch *cms =
        build_or_reuse_cms(ep, node, cms_addr, cfg.width, cfg.height);

    if (!cms) {
      return impls;
    }

    Module *module = new CMSIncrement(node, cms_addr, key);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    Context &ctx = new_ep->get_mutable_ctx();
    ctx.save_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch);

    TofinoContext *tofino_ctx = get_mutable_tofino_ctx(new_ep);
    tofino_ctx->place(new_ep, node, cms_addr, cms);

    EPLeaf leaf(ep_node, node->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }
};

} // namespace tofino
