#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class MeterInsert : public ControllerModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> success;

public:
  MeterInsert(const Node *node, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _success)
      : ControllerModule(ModuleType::Controller_MeterInsert, "MeterInsert", node), obj(_obj),
        keys(_keys), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    MeterInsert *cloned = new MeterInsert(node, obj, keys, success);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_success() const { return success; }
};

class MeterInsertFactory : public ControllerModuleFactory {
public:
  MeterInsertFactory()
      : ControllerModuleFactory(ModuleType::Controller_MeterInsert, "MeterInsert") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse