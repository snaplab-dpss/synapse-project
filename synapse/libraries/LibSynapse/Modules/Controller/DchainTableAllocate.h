#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DchainTableAllocate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> capacity;
  time_ns_t expiration_time;

public:
  DchainTableAllocate(const LibBDD::Node *node, addr_t _obj, klee::ref<klee::Expr> _key_size, klee::ref<klee::Expr> _capacity,
                      time_ns_t _expiration_time)
      : ControllerModule(ModuleType::Controller_DchainTableAllocate, "DchainTableAllocate", node), obj(_obj), key_size(_key_size),
        capacity(_capacity), expiration_time(_expiration_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new DchainTableAllocate(node, obj, key_size, capacity, expiration_time);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  time_ns_t get_expiration_time() const { return expiration_time; }
};

class DchainTableAllocateFactory : public ControllerModuleFactory {
public:
  DchainTableAllocateFactory() : ControllerModuleFactory(ModuleType::Controller_DchainTableAllocate, "DchainTableAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse