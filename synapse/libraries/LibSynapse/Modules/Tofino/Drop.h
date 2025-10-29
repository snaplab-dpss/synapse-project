#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class Drop : public TofinoModule {
public:
  Drop(const InstanceId _instance_id, const BDDNode *_node) : TofinoModule(ModuleType(ModuleCategory::Tofino_Drop, _instance_id), "Drop", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    Drop *cloned = new Drop(get_type().instance_id, node);
    return cloned;
  }
};

class DropFactory : public TofinoModuleFactory {
public:
  DropFactory(const InstanceId _instance_id) : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_Drop, _instance_id), "Drop") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
