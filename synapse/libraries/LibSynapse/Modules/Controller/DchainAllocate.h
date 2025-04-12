#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DchainAllocate : public ControllerModule {
private:
  addr_t dchain_addr;

public:
  DchainAllocate(const LibBDD::Node *_node, addr_t _dchain_addr)
      : ControllerModule(ModuleType::Controller_DchainAllocate, "DchainAllocate", _node), dchain_addr(_dchain_addr) {}

  DchainAllocate(const LibBDD::Node *_node, addr_t _dchain_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out)
      : ControllerModule(ModuleType::Controller_DchainAllocate, "DchainAllocate", _node), dchain_addr(_dchain_addr) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DchainAllocate(node, dchain_addr);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
};

class DchainAllocateFactory : public ControllerModuleFactory {
public:
  DchainAllocateFactory() : ControllerModuleFactory(ModuleType::Controller_DchainAllocate, "DchainAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse