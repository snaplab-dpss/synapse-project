#pragma once

#include "controller_module.h"

namespace synapse {
namespace controller {

class MapPut : public ControllerModule {
private:
  addr_t map_addr;
  addr_t key_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;

public:
  MapPut(const Node *node, addr_t _map_addr, addr_t _key_addr, klee::ref<klee::Expr> _key,
         klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType::Controller_MapPut, "MapPut", node),
        map_addr(_map_addr), key_addr(_key_addr), key(_key), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MapPut(node, map_addr, key_addr, key, value);
    return cloned;
  }

  addr_t get_map_addr() const { return map_addr; }
  addr_t get_key_addr() const { return key_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class MapPutFactory : public ControllerModuleFactory {
public:
  MapPutFactory() : ControllerModuleFactory(ModuleType::Controller_MapPut, "MapPut") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace controller
} // namespace synapse