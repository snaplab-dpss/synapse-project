#pragma once

#include <LibSynapse/Modules/Tofino/ComputeTable.h>

namespace LibSynapse {
namespace Tofino {

class PowerOfTwo : public ComputeTable {
public:
  PowerOfTwo(const BDDNode *_node, DS_ID _table_id, klee::ref<klee::Expr> _in, klee::ref<klee::Expr> _out)
      : ComputeTable(ModuleType::Tofino_PowerOfTwo, "PowerOfTwo", _node, _table_id, _in, _out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new PowerOfTwo(node, table_id, in, out); }
};

class PowerOfTwoFactory : public TofinoModuleFactory {
public:
  PowerOfTwoFactory() : TofinoModuleFactory(ModuleType::Tofino_PowerOfTwo, "PowerOfTwo") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
