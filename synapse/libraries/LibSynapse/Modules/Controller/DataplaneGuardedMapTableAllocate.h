#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneGuardedMapTableAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;

public:
  DataplaneGuardedMapTableAllocate(const LibBDD::Node *_node, addr_t _obj, klee::ref<klee::Expr> _key_size, klee::ref<klee::Expr> _value_size,
                                   klee::ref<klee::Expr> _capacity)
      : ControllerModule(ModuleType::Controller_DataplaneGuardedMapTableAllocate, "DataplaneGuardedMapTableAllocate", _node), obj(_obj),
        key_size(_key_size), value_size(_value_size), capacity(_capacity) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneGuardedMapTableAllocate(node, obj, key_size, value_size, capacity);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
  klee::ref<klee::Expr> get_value_size() const { return value_size; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
};

class DataplaneGuardedMapTableAllocateFactory : public ControllerModuleFactory {
public:
  DataplaneGuardedMapTableAllocateFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneGuardedMapTableAllocate, "DataplaneGuardedMapTableAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse