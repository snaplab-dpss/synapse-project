#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneDchainTableAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> capacity;
  time_ns_t expiration_time;

public:
  DataplaneDchainTableAllocate(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _key_size, klee::ref<klee::Expr> _capacity,
                               time_ns_t _expiration_time)
      : ControllerModule(ModuleType::Controller_DataplaneDchainTableAllocate, "DataplaneDchainTableAllocate", _node), obj(_obj), key_size(_key_size),
        capacity(_capacity), expiration_time(_expiration_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneDchainTableAllocate(node, obj, key_size, capacity, expiration_time);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  time_ns_t get_expiration_time() const { return expiration_time; }
};

class DataplaneDchainTableAllocateFactory : public ControllerModuleFactory {
public:
  DataplaneDchainTableAllocateFactory()
      : ControllerModuleFactory(ModuleType::Controller_DataplaneDchainTableAllocate, "DataplaneDchainTableAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse