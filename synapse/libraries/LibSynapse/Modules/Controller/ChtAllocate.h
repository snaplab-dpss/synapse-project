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
  ChtAllocate(ModuleType _type, const BDDNode *_node, addr_t _cht_addr, klee::ref<klee::Expr> _cht_height, klee::ref<klee::Expr> _backend_capacity)
      : ControllerModule(_type, "ChtAllocate", _node), cht_addr(_cht_addr), cht_height(_cht_height), backend_capacity(_backend_capacity) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new ChtAllocate(type, node, cht_addr, cht_height, backend_capacity);
    return cloned;
  }

  addr_t get_cht_addr() const { return cht_addr; }
  klee::ref<klee::Expr> get_cht_height() const { return cht_height; }
  klee::ref<klee::Expr> get_backend_capacity() const { return backend_capacity; }
};

class ChtAllocateFactory : public ControllerModuleFactory {
public:
  ChtAllocateFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_ChtAllocate, _instance_id), "ChtAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse