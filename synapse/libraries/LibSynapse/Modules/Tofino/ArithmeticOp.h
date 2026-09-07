#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

// One unrolled arithmetic operation (an op_* BDD node, see LibBDD/Unroll.h): a keyless table
// whose single action computes `value` (a <op> b) into metadata. Modeling it as a table gives
// the placer the stage the ALU operation costs.
class ArithmeticOp : public TofinoModule {
private:
  DS_ID table_id;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> out;

public:
  ArithmeticOp(const BDDNode *_node, DS_ID _table_id, klee::ref<klee::Expr> _value, klee::ref<klee::Expr> _out)
      : TofinoModule(ModuleType::Tofino_ArithmeticOp, "ArithmeticOp", _node), table_id(_table_id), value(_value), out(_out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new ArithmeticOp(node, table_id, value, out); }

  DS_ID get_table_id() const { return table_id; }
  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_out() const { return out; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {table_id}; }
};

class ArithmeticOpFactory : public TofinoModuleFactory {
public:
  ArithmeticOpFactory() : TofinoModuleFactory(ModuleType::Tofino_ArithmeticOp, "ArithmeticOp") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
