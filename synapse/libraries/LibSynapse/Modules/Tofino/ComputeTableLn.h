#pragma once

#include <LibSynapse/Modules/Tofino/ComputeTable.h>

namespace LibSynapse {
namespace Tofino {

class Ln : public ComputeTable {
public:
  Ln(const BDDNode *_node, DS_ID _table_id, klee::ref<klee::Expr> _in, klee::ref<klee::Expr> _out)
      : ComputeTable(ModuleType::Tofino_Ln, "Ln", _node, _table_id, _in, _out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new Ln(node, table_id, in, out); }
};

class LnFactory : public TofinoModuleFactory {
public:
  LnFactory() : TofinoModuleFactory(ModuleType::Tofino_Ln, "Ln") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
