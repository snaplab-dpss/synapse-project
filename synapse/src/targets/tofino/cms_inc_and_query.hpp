#pragma once

#include "tofino_module.hpp"

namespace synapse {
namespace tofino {

class CMSIncAndQuery : public TofinoModule {
private:
  DS_ID cms_id;
  addr_t cms_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;

public:
  CMSIncAndQuery(const Node *node, DS_ID _cms_id, addr_t _cms_addr, klee::ref<klee::Expr> _key,
                 klee::ref<klee::Expr> _min_estimate)
      : TofinoModule(ModuleType::Tofino_CMSIncAndQuery, "CMSIncAndQuery", node), cms_id(_cms_id), cms_addr(_cms_addr),
        key(_key), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override { return new CMSIncAndQuery(node, cms_id, cms_addr, key, min_estimate); }

  DS_ID get_cms_id() const { return cms_id; }
  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cms_id}; }
};

class CMSIncAndQueryFactory : public TofinoModuleFactory {
public:
  CMSIncAndQueryFactory() : TofinoModuleFactory(ModuleType::Tofino_CMSIncAndQuery, "CMSIncAndQuery") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse