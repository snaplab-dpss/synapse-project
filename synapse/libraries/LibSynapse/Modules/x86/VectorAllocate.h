#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class VectorAllocate : public x86Module {
private:
  klee::ref<klee::Expr> elem_size;
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> vector_out;
  symbol_t success;

public:
  VectorAllocate(const std::string &_instance_id, const BDDNode *_node, klee::ref<klee::Expr> _elem_size, klee::ref<klee::Expr> _capacity,
                 klee::ref<klee::Expr> _vector_out, symbol_t _success)
      : x86Module(ModuleType(ModuleCategory::x86_VectorAllocate, _instance_id), "VectorAllocate", _node), elem_size(_elem_size), capacity(_capacity),
        vector_out(_vector_out), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorAllocate(get_type().instance_id, node, elem_size, capacity, vector_out, success);
    return cloned;
  }

  klee::ref<klee::Expr> get_elem_size() const { return elem_size; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  klee::ref<klee::Expr> get_vector_out() const { return vector_out; }
  symbol_t get_success() const { return success; }
};

class VectorAllocateFactory : public x86ModuleFactory {
public:
  VectorAllocateFactory(const std::string &_instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_VectorAllocate, _instance_id), "VectorAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};
} // namespace x86

} // namespace LibSynapse
