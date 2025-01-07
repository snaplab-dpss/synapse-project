#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

using tofino::DS_ID;

class MapRegisterWrite : public ControllerModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;

public:
  MapRegisterWrite(const Node *node, DS_ID _id, addr_t _obj,
                   const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType::Controller_MapRegisterWrite, "MapRegisterWrite", node),
        id(_id), obj(_obj), keys(_keys), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    MapRegisterWrite *cloned = new MapRegisterWrite(node, id, obj, keys, value);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class MapRegisterWriteFactory : public ControllerModuleFactory {
public:
  MapRegisterWriteFactory()
      : ControllerModuleFactory(ModuleType::Controller_MapRegisterWrite, "MapRegisterWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse