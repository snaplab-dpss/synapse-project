#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class TableLookup : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<symbol_t> found;

public:
  TableLookup(const Node *node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
              const std::vector<klee::ref<klee::Expr>> &_values, const std::optional<symbol_t> &_found)
      : ControllerModule(ModuleType::Controller_TableLookup, "TableLookup", node), obj(_obj), keys(_keys), values(_values), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TableLookup(node, obj, keys, values, found);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const std::vector<klee::ref<klee::Expr>> &get_values() const { return values; }
  const std::optional<symbol_t> &get_found() const { return found; }
};

class TableLookupFactory : public ControllerModuleFactory {
public:
  TableLookupFactory() : ControllerModuleFactory(ModuleType::Controller_TableLookup, "TableLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse