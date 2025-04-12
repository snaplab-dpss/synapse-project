#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class MapAllocate : public ControllerModule {
private:
  addr_t map_addr;
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> key_size;

public:
  MapAllocate(const LibBDD::Node *_node, addr_t _map_addr, klee::ref<klee::Expr> _capacity, klee::ref<klee::Expr> _key_size)
      : ControllerModule(ModuleType::Controller_MapAllocate, "MapAllocate", _node), map_addr(_map_addr), capacity(_capacity), key_size(_key_size) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new MapAllocate(node, map_addr, capacity, key_size);
    return cloned;
  }

  addr_t get_map_addr() const { return map_addr; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
};

class MapAllocateFactory : public ControllerModuleFactory {
public:
  MapAllocateFactory() : ControllerModuleFactory(ModuleType::Controller_MapAllocate, "MapAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse