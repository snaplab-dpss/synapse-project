#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class Drop : public x86Module {
public:
  Drop(const std::string &_instance_id, const BDDNode *_node) : x86Module(ModuleType(ModuleCategory::x86_Drop, _instance_id), "Drop", _node) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    Drop *cloned = new Drop(get_type().instance_id, node);
    return cloned;
  }
};

class DropFactory : public x86ModuleFactory {
public:
  DropFactory(const std::string &_instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_Drop, _instance_id), "Drop") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse