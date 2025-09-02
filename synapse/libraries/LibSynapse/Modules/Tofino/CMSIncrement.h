#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class CMSIncrement : public TofinoModule {
private:
  DS_ID cms_id;
  addr_t cms_addr;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  CMSIncrement(const std::string &_instance_id, const BDDNode *_node, DS_ID _cms_id, addr_t _cms_addr,
               const std::vector<klee::ref<klee::Expr>> &_keys)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_CMSIncrement, _instance_id), "CMSIncrement", _node), cms_id(_cms_id), cms_addr(_cms_addr),
        keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new CMSIncrement(get_type().instance_id, node, cms_id, cms_addr, keys); }

  DS_ID get_cms_id() const { return cms_id; }
  addr_t get_cms_addr() const { return cms_addr; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cms_id}; }
};

class CMSIncrementFactory : public TofinoModuleFactory {
public:
  CMSIncrementFactory(const std::string &_instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_CMSIncrement, _instance_id), "CMSIncrement") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse