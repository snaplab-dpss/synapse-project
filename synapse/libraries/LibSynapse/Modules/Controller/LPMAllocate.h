#pragma once

// The LPM kept by the controller itself: libnf's, the very structure the NF's C code runs (as the
// controller's token bucket), rather than the switch's ternary table.

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class LPMAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> key_size;

public:
  LPMAllocate(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _capacity, klee::ref<klee::Expr> _key_size)
      : ControllerModule(ModuleType::Controller_LPMAllocate, "LPMAllocate", _node), obj(_obj), capacity(_capacity), key_size(_key_size) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    LPMAllocate *cloned = new LPMAllocate(node, obj, capacity, key_size);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
};

class LPMAllocateFactory : public ControllerModuleFactory {
public:
  LPMAllocateFactory() : ControllerModuleFactory(ModuleType::Controller_LPMAllocate, "LPMAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
