#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class HHTableDelete : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;

public:
  HHTableDelete(const Node *node, addr_t _obj,
                const std::vector<klee::ref<klee::Expr>> &_keys)
      : TofinoCPUModule(ModuleType::TofinoCPU_HHTableDelete, "HHTableDelete", node),
        obj(_obj), keys(_keys) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const {
    HHTableDelete *cloned = new HHTableDelete(node, obj, keys);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
};

class HHTableDeleteFactory : public TofinoCPUModuleFactory {
public:
  HHTableDeleteFactory()
      : TofinoCPUModuleFactory(ModuleType::TofinoCPU_HHTableDelete, "HHTableDelete") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
