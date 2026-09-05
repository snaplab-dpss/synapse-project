#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// Controller-side (CPU) `divide`, evaluated with libnf's implementation (embedded in
// libsycon). Stateless: computes the result from its inputs and binds it for downstream
// use. Used when the operation is offloaded to the controller.
class Divide : public ControllerModule {
private:
  klee::ref<klee::Expr> numerator;
  klee::ref<klee::Expr> denominator;
  klee::ref<klee::Expr> out;

public:
  Divide(const BDDNode *_node, klee::ref<klee::Expr> _numerator, klee::ref<klee::Expr> _denominator, klee::ref<klee::Expr> _out)
      : ControllerModule(ModuleType::Controller_Divide, "Divide", _node), numerator(_numerator), denominator(_denominator), out(_out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new Divide(node, numerator, denominator, out); }

  klee::ref<klee::Expr> get_numerator() const { return numerator; }
  klee::ref<klee::Expr> get_denominator() const { return denominator; }
  klee::ref<klee::Expr> get_out() const { return out; }
};

class DivideFactory : public ControllerModuleFactory {
public:
  DivideFactory() : ControllerModuleFactory(ModuleType::Controller_Divide, "Divide") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
