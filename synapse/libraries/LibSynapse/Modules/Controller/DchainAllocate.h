#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DchainAllocate : public ControllerModule {
private:
  addr_t dchain_addr;

public:
  DchainAllocate(const InstanceId _instance_id, const BDDNode *_node, addr_t _dchain_addr)
      : ControllerModule(ModuleType(ModuleCategory::Controller_DchainAllocate, _instance_id), "DchainAllocate", _node), dchain_addr(_dchain_addr) {}

  DchainAllocate(ModuleType _type, const BDDNode *_node, addr_t _dchain_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out)
      : ControllerModule(_type, "DchainAllocate", _node), dchain_addr(_dchain_addr) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DchainAllocate(get_type().instance_id, node, dchain_addr);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
};

class DchainAllocateFactory : public ControllerModuleFactory {
public:
  DchainAllocateFactory(const InstanceId _instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DchainAllocate, _instance_id), "DchainAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse