#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class VectorAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> elem_size;
  klee::ref<klee::Expr> capacity;

public:
  VectorAllocate(const InstanceId _instance_id, const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _elem_size, klee::ref<klee::Expr> _capacity)
      : ControllerModule(ModuleType(ModuleCategory::Controller_VectorAllocate, _instance_id), "VectorAllocate", _node), obj(_obj),
        elem_size(_elem_size), capacity(_capacity) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorAllocate(get_type().instance_id, node, obj, elem_size, capacity);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_elem_size() const { return elem_size; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
};

class VectorAllocateFactory : public ControllerModuleFactory {
public:
  VectorAllocateFactory(const InstanceId _instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_VectorAllocate, _instance_id), "VectorAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse