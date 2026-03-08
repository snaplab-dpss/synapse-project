#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class SetDeviceInfo : public TofinoModule {
private:
  klee::ref<klee::Expr> device;
  symbol_t device_symbol;

public:
  SetDeviceInfo(const BDDNode *_node, klee::ref<klee::Expr> _device, symbol_t _device_symbol)
      : TofinoModule(ModuleType::Tofino_SetDeviceInfo, "SetDeviceInfo", _node), device(_device), device_symbol(_device_symbol) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    SetDeviceInfo *cloned = new SetDeviceInfo(node, device, device_symbol);
    return cloned;
  }

  klee::ref<klee::Expr> get_device() const { return device; }
  symbol_t get_device_symbol() const { return device_symbol; }
};

class SetDeviceInfoFactory : public TofinoModuleFactory {
public:
  SetDeviceInfoFactory() : TofinoModuleFactory(ModuleType::Tofino_SetDeviceInfo, "SetDeviceInfo") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};
} // namespace Tofino
} // namespace LibSynapse
