#pragma once

#include "tofino_module.h"

namespace tofino {

class CMSQuery : public TofinoModule {
private:
  DS_ID cms_id;
  addr_t cms_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;

public:
  CMSQuery(const Node *node, DS_ID _cms_id, addr_t _cms_addr,
           klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _min_estimate)
      : TofinoModule(ModuleType::Tofino_CMSQuery, "CMSQuery", node),
        cms_id(_cms_id), cms_addr(_cms_addr), key(_key),
        min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    return new CMSQuery(node, cms_id, cms_addr, key, min_estimate);
  }

  DS_ID get_cms_id() const { return cms_id; }
  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {cms_id};
  }
};

class CMSQueryGenerator : public TofinoModuleGenerator {
public:
  CMSQueryGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_CMSQuery, "CMSQuery") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::Call) {
      return std::nullopt;
    }

    const Call *call_node = static_cast<const Call *>(node);
    const call_t &call = call_node->get_call();

    if (call.function_name != "cms_count_min") {
      return std::nullopt;
    }

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;

    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    if (!ctx.can_impl_ds(cms_addr, DSImpl::Tofino_CountMinSketch)) {
      return std::nullopt;
    }

    const cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_addr);
    std::vector<klee::ref<klee::Expr>> keys = Table::build_keys(key);

    if (!can_build_or_reuse_cms(ep, node, cms_addr, keys, cfg.width,
                                cfg.height)) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(cms_addr, DSImpl::Tofino_CountMinSketch);

    return spec_impl_t(decide(ep, node), new_ctx);
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::Call) {
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
    ASSERT(found, "Symbol min_estimate not found");

    if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::Tofino_CountMinSketch)) {
      return impls;
    }

    const cms_config_t &cfg = ep->get_ctx().get_cms_config(cms_addr);
    std::vector<klee::ref<klee::Expr>> keys = Table::build_keys(key);

    CountMinSketch *cms =
        build_or_reuse_cms(ep, node, cms_addr, keys, cfg.width, cfg.height);

    if (!cms) {
      return impls;
    }

    Module *module =
        new CMSQuery(node, cms->id, cms_addr, key, min_estimate.expr);
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
