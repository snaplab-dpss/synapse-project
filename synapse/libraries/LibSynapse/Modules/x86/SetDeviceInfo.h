#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class SetDeviceInfo : public x86Module {
private:
  klee::ref<klee::Expr> device;

public:
  SetDeviceInfo(const BDDNode *_node, klee::ref<klee::Expr> _device)
      : x86Module(ModuleType::x86_SetDeviceInfo, "SetDeviceInfo", _node), device(_device) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    SetDeviceInfo *cloned = new SetDeviceInfo(node, device);
    return cloned;
  }

  klee::ref<klee::Expr> get_device() const { return device; }
};

class SetDeviceInfoFactory : public x86ModuleFactory {
public:
  SetDeviceInfoFactory() : x86ModuleFactory(ModuleType::x86_SetDeviceInfo, "SetDeviceInfo") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};
} // namespace x86
} // namespace LibSynapse
