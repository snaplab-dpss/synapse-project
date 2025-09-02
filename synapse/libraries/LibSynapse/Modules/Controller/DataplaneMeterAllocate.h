#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneMeterAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> rate;
  klee::ref<klee::Expr> burst;
  klee::ref<klee::Expr> key_size;

public:
  DataplaneMeterAllocate(const std::string &_instance_id, const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _capacity,
                         klee::ref<klee::Expr> _rate, klee::ref<klee::Expr> _burst, klee::ref<klee::Expr> _key_size)
      : ControllerModule(ModuleType(ModuleCategory::Controller_DataplaneMeterAllocate, _instance_id), "DataplaneMeterAllocate", _node), obj(_obj),
        capacity(_capacity), rate(_rate), burst(_burst), key_size(_key_size) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    DataplaneMeterAllocate *cloned = new DataplaneMeterAllocate(get_type().instance_id, node, obj, capacity, rate, burst, key_size);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  klee::ref<klee::Expr> get_rate() const { return rate; }
  klee::ref<klee::Expr> get_burst() const { return burst; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
};

class DataplaneMeterAllocateFactory : public ControllerModuleFactory {
public:
  DataplaneMeterAllocateFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_DataplaneMeterAllocate, _instance_id), "DataplaneMeterAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse