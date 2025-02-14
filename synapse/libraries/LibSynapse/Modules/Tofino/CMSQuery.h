#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class CMSQuery : public TofinoModule {
private:
  DS_ID cms_id;
  addr_t cms_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> min_estimate;

public:
  CMSQuery(const LibBDD::Node *node, DS_ID _cms_id, addr_t _cms_addr, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _min_estimate)
      : TofinoModule(ModuleType::Tofino_CMSQuery, "CMSQuery", node), cms_id(_cms_id), cms_addr(_cms_addr), key(_key),
        min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override { return new CMSQuery(node, cms_id, cms_addr, key, min_estimate); }

  DS_ID get_cms_id() const { return cms_id; }
  addr_t get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cms_id}; }
};

class CMSQueryFactory : public TofinoModuleFactory {
public:
  CMSQueryFactory() : TofinoModuleFactory(ModuleType::Tofino_CMSQuery, "CMSQuery") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse