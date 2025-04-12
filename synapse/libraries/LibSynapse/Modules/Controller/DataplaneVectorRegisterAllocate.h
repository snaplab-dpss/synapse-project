#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneVectorRegisterAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> elem_size;
  klee::ref<klee::Expr> capacity;

public:
  DataplaneVectorRegisterAllocate(const LibBDD::Node *_node, addr_t _obj, klee::ref<klee::Expr> _elem_size, klee::ref<klee::Expr> _capacity)
      : ControllerModule(ModuleType::Controller_DataplaneVectorRegisterAllocate, "DataplaneVectorRegisterAllocate", _node), obj(_obj),
        elem_size(_elem_size), capacity(_capacity) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneVectorRegisterAllocate(node, obj, elem_size, capacity);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_elem_size() const { return elem_size; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
};

class DataplaneVectorRegisterAllocateFactory : public ControllerModuleFactory {
public:
  DataplaneVectorRegisterAllocateFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneVectorRegisterAllocate, "DataplaneVectorRegisterAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse