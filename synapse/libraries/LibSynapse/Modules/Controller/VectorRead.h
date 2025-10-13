#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class VectorRead : public ControllerModule {
private:
  addr_t vector_addr;
  klee::ref<klee::Expr> index;
  addr_t value_addr;
  klee::ref<klee::Expr> value;

public:
  VectorRead(const InstanceId _instance_id, const BDDNode *_node, addr_t _vector_addr, klee::ref<klee::Expr> _index, addr_t _value_addr,
             klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType(ModuleCategory::Controller_VectorRead, _instance_id), "VectorRead", _node), vector_addr(_vector_addr),
        index(_index), value_addr(_value_addr), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorRead(get_type().instance_id, node, vector_addr, index, value_addr, value);
    return cloned;
  }

  addr_t get_vector_addr() const { return vector_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  addr_t get_value_addr() const { return value_addr; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class VectorReadFactory : public ControllerModuleFactory {
public:
  VectorReadFactory(const InstanceId _instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_VectorRead, _instance_id), "VectorRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse