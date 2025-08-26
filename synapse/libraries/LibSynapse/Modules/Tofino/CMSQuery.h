#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class CMSQuery : public TofinoModule {
private:
  DS_ID cms_id;
  addr_t cms_addr;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> min_estimate;

public:
  CMSQuery(ModuleType _type, const BDDNode *_node, DS_ID _cms_id, addr_t _cms_addr, const std::vector<klee::ref<klee::Expr>> &_keys,
           klee::ref<klee::Expr> _min_estimate)
      : TofinoModule(_type, "CMSQuery", _node), cms_id(_cms_id), cms_addr(_cms_addr), keys(_keys), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new CMSQuery(type, node, cms_id, cms_addr, keys, min_estimate); }

  DS_ID get_cms_id() const { return cms_id; }
  addr_t get_cms_addr() const { return cms_addr; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cms_id}; }
};

class CMSQueryFactory : public TofinoModuleFactory {
public:
  CMSQueryFactory(const std::string &_instance_id) : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_CMSQuery, _instance_id), "CMSQuery") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse