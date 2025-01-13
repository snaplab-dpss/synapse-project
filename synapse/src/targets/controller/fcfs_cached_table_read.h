#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

using tofino::DS_ID;

class FCFSCachedTableRead : public ControllerModule {
private:
  DS_ID id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<symbol_t> found;

public:
  FCFSCachedTableRead(const Node *node, DS_ID _id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
                      klee::ref<klee::Expr> _value, const std::optional<symbol_t> &_found)
      : ControllerModule(ModuleType::Controller_FCFSCachedTableRead, "FCFSCachedTableRead", node), id(_id), obj(_obj),
        keys(_keys), value(_value), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableRead(node, id, obj, keys, value, found);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const std::optional<symbol_t> &get_found() const { return found; }
};

class FCFSCachedTableReadFactory : public ControllerModuleFactory {
public:
  FCFSCachedTableReadFactory()
      : ControllerModuleFactory(ModuleType::Controller_FCFSCachedTableRead, "FCFSCachedTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse