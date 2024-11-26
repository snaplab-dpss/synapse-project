#pragma once

#include "tofino_module.h"

namespace tofino {

class CMSIncAndQuery : public TofinoModule {
private:
  addr_t cms_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;

public:
  CMSIncAndQuery(const Node *node, addr_t _cms_addr, klee::ref<klee::Expr> _key,
                 klee::ref<klee::Expr> _min_estimate)
      : TofinoModule(ModuleType::Tofino_CMSIncAndQuery, "CMSIncAndQuery", node),
        cms_addr(_cms_addr), key(_key), min_estimate(_min_estimate) {}

  virtual void visit(EPVisitor &visitor, const EP *ep,
                     const EPNode *ep_node) const override {
    visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    return new CMSIncAndQuery(node, cms_addr, key, min_estimate);
  }

  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    // FIXME:
    return {};
  }
};

class CMSIncAndQueryGenerator : public TofinoModuleGenerator {
public:
  CMSIncAndQueryGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_CMSIncAndQuery,
                              "CMSIncAndQuery") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override {
    if (node->get_type() != NodeType::CALL) {
      return std::nullopt;
    }

    const Call *cms_increment = static_cast<const Call *>(node);
    const call_t &call = cms_increment->get_call();

    if (!is_inc_and_query_cms(ep, cms_increment)) {
      return std::nullopt;
    }

    const Call *count_min = static_cast<const Call *>(node->get_next());

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    if (!ctx.can_impl_ds(cms_addr, DSImpl::Tofino_CMS)) {
      return std::nullopt;
    }

    Context new_ctx = ctx;
    new_ctx.save_ds_impl(cms_addr, DSImpl::Tofino_CMS);

    spec_impl_t spec_impl = spec_impl_t(decide(ep, node), new_ctx);
    spec_impl.skip.insert(count_min->get_id());

    return spec_impl;
  }

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override {
    std::vector<impl_t> impls;

    if (node->get_type() != NodeType::CALL) {
      return impls;
    }

    const Call *cms_increment = static_cast<const Call *>(node);

    if (!is_inc_and_query_cms(ep, cms_increment)) {
      return impls;
    }

    const Call *count_min = static_cast<const Call *>(node->get_next());
    const call_t &call = count_min->get_call();

    klee::ref<klee::Expr> cms_addr_expr = call.args.at("cms").expr;
    klee::ref<klee::Expr> key = call.args.at("key").in;

    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    symbols_t symbols = count_min->get_locally_generated_symbols();
    symbol_t min_estimate;
    bool found = get_symbol(symbols, "min_estimate", min_estimate);
    assert(found && "Symbol min_estimate not found");

    if (!ep->get_ctx().can_impl_ds(cms_addr, DSImpl::Tofino_CMS)) {
      return impls;
    }

    // TODO: implement the actual data structure.

    Module *module = new CMSIncAndQuery(node, cms_addr, key, min_estimate.expr);
    EPNode *ep_node = new EPNode(module);

    EP *new_ep = new EP(*ep);
    impls.push_back(implement(ep, node, new_ep));

    new_ep->get_mutable_ctx().save_ds_impl(cms_addr, DSImpl::Tofino_CMS);

    EPLeaf leaf(ep_node, count_min->get_next());
    new_ep->process_leaf(ep_node, {leaf});

    return impls;
  }

private:
  bool is_inc_and_query_cms(const EP *ep, const Call *cms_increment) const {
    if (cms_increment->get_call().function_name != "cms_increment") {
      return false;
    }

    klee::ref<klee::Expr> cms_addr_expr =
        cms_increment->get_call().args.at("cms").expr;
    addr_t cms_addr = expr_addr_to_obj_addr(cms_addr_expr);

    const Node *next = cms_increment->get_next();

    if (!next || next->get_type() != NodeType::CALL) {
      return false;
    }

    const Call *cms_count_min = static_cast<const Call *>(next);

    if (cms_count_min->get_call().function_name != "cms_count_min") {
      return false;
    }

    klee::ref<klee::Expr> cms_addr_expr2 =
        cms_count_min->get_call().args.at("cms").expr;
    addr_t cms_addr2 = expr_addr_to_obj_addr(cms_addr_expr2);

    return cms_addr == cms_addr2;
  };
};

} // namespace tofino
