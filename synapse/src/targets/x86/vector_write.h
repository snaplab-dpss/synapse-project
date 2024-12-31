#pragma once

#include "x86_module.h"

namespace x86 {

class VectorWrite : public x86Module {
private:
  addr_t vector_addr;
  klee::ref<klee::Expr> index;
  addr_t value_addr;
  std::vector<mod_t> modifications;

public:
  VectorWrite(const Node *node, addr_t _vector_addr,
              klee::ref<klee::Expr> _index, addr_t _value_addr,
              const std::vector<mod_t> &_modifications)
      : x86Module(ModuleType::x86_VectorWrite, "VectorWrite", node),
        vector_addr(_vector_addr), index(_index), value_addr(_value_addr),
        modifications(_modifications) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new VectorWrite(node, vector_addr, index, value_addr, modifications);
    return cloned;
  }

  addr_t get_vector_addr() const { return vector_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  addr_t get_value_addr() const { return value_addr; }

  const std::vector<mod_t> &get_modifications() const { return modifications; }
};

class VectorWriteGenerator : public x86ModuleGenerator {
public:
  VectorWriteGenerator()
      : x86ModuleGenerator(ModuleType::x86_VectorWrite, "VectorWrite") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace x86