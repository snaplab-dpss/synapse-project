#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class VectorRead : public x86Module {
private:
  klee::ref<klee::Expr> vector_addr_expr;
  klee::ref<klee::Expr> vector_addr_expr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> value_addr;
  klee::ref<klee::Expr> value_addr;
  klee::ref<klee::Expr> value;

public:
  VectorRead(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _vector_addr_expr, klee::ref<klee::Expr> _index,
             klee::ref<klee::Expr> _value_addr, klee::ref<klee::Expr> _value)
      : x86Module(ModuleType(ModuleCategory::x86_VectorRead, _instance_id), "VectorRead", _node), vector_addr_expr(_vector_addr_expr), index(_index),
        value_addr(_value_addr), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new VectorRead(get_type().instance_id, node, vector_addr_expr, index, value_addr, value);
    Module *cloned = new VectorRead(get_type().instance_id, node, vector_addr_expr, index, value_addr, value);
    return cloned;
  }

  klee::ref<klee::Expr> get_vector_addr_expr() const { return vector_addr_expr; }
  klee::ref<klee::Expr> get_vector_addr_expr() const { return vector_addr_expr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_value_addr() const { return value_addr; }
  klee::ref<klee::Expr> get_value_addr() const { return value_addr; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class VectorReadFactory : public x86ModuleFactory {
public:
  VectorReadFactory(const InstanceId _instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_VectorRead, _instance_id), "VectorRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse