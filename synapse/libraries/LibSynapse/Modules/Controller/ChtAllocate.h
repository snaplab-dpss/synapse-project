#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class ChtAllocate : public ControllerModule {
private:
  addr_t cht_addr;
  klee::ref<klee::Expr> cht_height;
  klee::ref<klee::Expr> backend_capacity;

public:
  ChtAllocate(const LibBDD::Node *_node, addr_t _cht_addr, klee::ref<klee::Expr> _cht_height, klee::ref<klee::Expr> _backend_capacity)
      : ControllerModule(ModuleType::Controller_ChtAllocate, "ChtAllocate", _node), cht_addr(_cht_addr), cht_height(_cht_height),
        backend_capacity(_backend_capacity) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new ChtAllocate(node, cht_addr, cht_height, backend_capacity);
    return cloned;
  }

  addr_t get_cht_addr() const { return cht_addr; }
  klee::ref<klee::Expr> get_cht_height() const { return cht_height; }
  klee::ref<klee::Expr> get_backend_capacity() const { return backend_capacity; }
};

class ChtAllocateFactory : public ControllerModuleFactory {
public:
  ChtAllocateFactory() : ControllerModuleFactory(ModuleType::Controller_ChtAllocate, "ChtAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse