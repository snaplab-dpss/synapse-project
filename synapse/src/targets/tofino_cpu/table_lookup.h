#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class TableLookup : public TofinoCPUModule {
private:
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<symbol_t> found;

public:
  TableLookup(const Node *node, addr_t _obj,
              const std::vector<klee::ref<klee::Expr>> &_keys,
              const std::vector<klee::ref<klee::Expr>> &_values,
              const std::optional<symbol_t> &_found)
      : TofinoCPUModule(ModuleType::TofinoCPU_TableLookup, "TableLookup", node),
        obj(_obj), keys(_keys), values(_values), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TableLookup(node, obj, keys, values, found);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const std::vector<klee::ref<klee::Expr>> &get_values() const {
    return values;
  }
  const std::optional<symbol_t> &get_found() const { return found; }
};

class TableLookupGenerator : public TofinoCPUModuleGenerator {
public:
  TableLookupGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_TableLookup,
                                 "TableLookup") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino_cpu
