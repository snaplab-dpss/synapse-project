#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// Controller-side (CPU) `power_of_two`, evaluated with libnf's implementation (embedded in
// libsycon). Stateless: computes the result from its input and binds it for downstream
// use. Used when the operation is offloaded to the controller.
class PowerOfTwo : public ControllerModule {
private:
  klee::ref<klee::Expr> exponent;
  klee::ref<klee::Expr> out;

public:
  PowerOfTwo(const BDDNode *_node, klee::ref<klee::Expr> _exponent, klee::ref<klee::Expr> _out)
      : ControllerModule(ModuleType::Controller_PowerOfTwo, "PowerOfTwo", _node), exponent(_exponent), out(_out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new PowerOfTwo(node, exponent, out); }

  klee::ref<klee::Expr> get_exponent() const { return exponent; }
  klee::ref<klee::Expr> get_out() const { return out; }
};

class PowerOfTwoFactory : public ControllerModuleFactory {
public:
  PowerOfTwoFactory() : ControllerModuleFactory(ModuleType::Controller_PowerOfTwo, "PowerOfTwo") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
