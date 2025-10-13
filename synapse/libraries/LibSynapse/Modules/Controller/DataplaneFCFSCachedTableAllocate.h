#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneFCFSCachedTableAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> value_size;
  klee::ref<klee::Expr> capacity;

public:
  DataplaneFCFSCachedTableAllocate(const InstanceId _instance_id, const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _key_size,
                                   klee::ref<klee::Expr> _value_size, klee::ref<klee::Expr> _capacity)
      : ControllerModule(ModuleType(ModuleCategory::Controller_DataplaneFCFSCachedTableAllocate, _instance_id), "DataplaneFCFSCachedTableAllocate",
                         _node),
        obj(_obj), key_size(_key_size), value_size(_value_size), capacity(_capacity) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneFCFSCachedTableAllocate *cloned =
        new DataplaneFCFSCachedTableAllocate(get_type().instance_id, node, obj, key_size, value_size, capacity);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
  klee::ref<klee::Expr> get_value_size() const { return value_size; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
};

class DataplaneFCFSCachedTableAllocateFactory : public ControllerModuleFactory {
public:
  DataplaneFCFSCachedTableAllocateFactory(const InstanceId _instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DataplaneFCFSCachedTableAllocate, _instance_id),
                                "DataplaneFCFSCachedTableAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse