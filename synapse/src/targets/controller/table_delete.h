#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

using tofino::DS;
using tofino::DSType;
using tofino::Table;
using tofino::TofinoContext;

class TableDelete : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  TableDelete(const Node *node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys)
      : ControllerModule(ModuleType::Controller_TableDelete, "TableDelete", node), obj(_obj), keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    TableDelete *cloned = new TableDelete(node, obj, keys);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
};

class TableDeleteFactory : public ControllerModuleFactory {
public:
  TableDeleteFactory() : ControllerModuleFactory(ModuleType::Controller_TableDelete, "TableDelete") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse