#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class HHTableRead : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> map_has_this_key;
  klee::ref<klee::Expr> min_estimate;

public:
  HHTableRead(const Node *node, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _value, klee::ref<klee::Expr> _map_has_this_key,
              klee::ref<klee::Expr> _min_estimate)
      : TofinoCPUModule(ModuleType::TofinoCPU_HHTableRead, "HHTableRead", node),
        obj(_obj), keys(_keys), value(_value), map_has_this_key(_map_has_this_key),
        min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new HHTableRead(node, obj, keys, value, map_has_this_key, min_estimate);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_hit() const { return map_has_this_key; }
  klee::ref<klee::Expr> get_min_estimate() const { return min_estimate; }
};

class HHTableReadFactory : public TofinoCPUModuleFactory {
public:
  HHTableReadFactory()
      : TofinoCPUModuleFactory(ModuleType::TofinoCPU_HHTableRead, "HHTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino_cpu
