#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>
#include <LibCore/Expr.h>

namespace LibSynapse {
namespace x86 {

using LibCore::expr_mod_t;

class VectorWrite : public x86Module {
private:
  addr_t vector_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> value_addr;
  klee::ref<klee::Expr> value;
  std::vector<expr_mod_t> modifications;

public:
  VectorWrite(const InstanceId _instance_id, const BDDNode *_node, addr_t _vector_addr, klee::ref<klee::Expr> _index,
              klee::ref<klee::Expr> _value_addr, klee::ref<klee::Expr> _value, const std::vector<expr_mod_t> &_modifications)
      : x86Module(ModuleType(ModuleCategory::x86_VectorWrite, _instance_id), "VectorWrite", _node), vector_addr(_vector_addr), index(_index),
        value_addr(_value_addr), value(_value), modifications(_modifications) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorWrite(get_type().instance_id, node, vector_addr, index, value_addr, value, modifications);
    return cloned;
  }

  addr_t get_vector_addr() const { return vector_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_value_addr() const { return value_addr; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const std::vector<expr_mod_t> &get_modifications() const { return modifications; }
};

class VectorWriteFactory : public x86ModuleFactory {
public:
  VectorWriteFactory(const InstanceId _instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_VectorWrite, _instance_id), "VectorWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse