#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class VectorRegisterAllocate : public ControllerModule {
private:
  addr_t vector_addr;
  klee::ref<klee::Expr> elem_size;
  klee::ref<klee::Expr> capacity;

public:
  VectorRegisterAllocate(const LibBDD::Node *node, addr_t _vector_addr, klee::ref<klee::Expr> _elem_size, klee::ref<klee::Expr> _capacity)
      : ControllerModule(ModuleType::Controller_VectorRegisterAllocate, "VectorRegisterAllocate", node), vector_addr(_vector_addr),
        elem_size(_elem_size), capacity(_capacity) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new VectorRegisterAllocate(node, vector_addr, elem_size, capacity);
    return cloned;
  }

  addr_t get_vector_addr() const { return vector_addr; }
  klee::ref<klee::Expr> get_elem_size() const { return elem_size; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
};

class VectorRegisterAllocateFactory : public ControllerModuleFactory {
public:
  VectorRegisterAllocateFactory() : ControllerModuleFactory(ModuleType::Controller_VectorRegisterAllocate, "VectorRegisterAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse