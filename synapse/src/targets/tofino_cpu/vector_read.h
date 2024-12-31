#pragma once

#include "tofino_cpu_module.h"

namespace tofino_cpu {

class VectorRead : public TofinoCPUModule {
private:
  addr_t vector_addr;
  klee::ref<klee::Expr> index;
  addr_t value_addr;
  klee::ref<klee::Expr> value;

public:
  VectorRead(const Node *node, addr_t _vector_addr,
             klee::ref<klee::Expr> _index, addr_t _value_addr,
             klee::ref<klee::Expr> _value)
      : TofinoCPUModule(ModuleType::TofinoCPU_VectorRead, "VectorRead", node),
        vector_addr(_vector_addr), index(_index), value_addr(_value_addr),
        value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new VectorRead(node, vector_addr, index, value_addr, value);
    return cloned;
  }

  addr_t get_vector_addr() const { return vector_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  addr_t get_value_addr() const { return value_addr; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class VectorReadGenerator : public TofinoCPUModuleGenerator {
public:
  VectorReadGenerator()
      : TofinoCPUModuleGenerator(ModuleType::TofinoCPU_VectorRead,
                                 "VectorRead") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino_cpu
