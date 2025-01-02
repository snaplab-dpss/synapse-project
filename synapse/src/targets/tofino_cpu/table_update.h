#pragma once

#include "tofino_cpu_module.h"

namespace synapse {
namespace tofino_cpu {

class TableUpdate : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;

public:
  TableUpdate(const Node *node, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              const std::vector<klee::ref<klee::Expr>> &_values)
      : TofinoCPUModule(ModuleType::TofinoCPU_TableUpdate, "TableUpdate", node),
        obj(_obj), keys(_keys), values(_values) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    TableUpdate *cloned = new TableUpdate(node, obj, keys, values);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const std::vector<klee::ref<klee::Expr>> &get_values() const { return values; }
};

class TableUpdateFactory : public TofinoCPUModuleFactory {
public:
  TableUpdateFactory()
      : TofinoCPUModuleFactory(ModuleType::TofinoCPU_TableUpdate, "TableUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
} // namespace synapse