#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// Controller-side (CPU) `ln`, evaluated with libnf's implementation (embedded in
// libsycon). Stateless: computes the result from its inputs and binds it for downstream
// use. Used when the operation is offloaded to the controller.
class Ln : public ControllerModule {
private:
  klee::ref<klee::Expr> x;
  klee::ref<klee::Expr> scale;
  klee::ref<klee::Expr> out;

public:
  Ln(const BDDNode *_node, klee::ref<klee::Expr> _x, klee::ref<klee::Expr> _scale, klee::ref<klee::Expr> _out)
      : ControllerModule(ModuleType::Controller_Ln, "Ln", _node), x(_x), scale(_scale), out(_out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new Ln(node, x, scale, out); }

  klee::ref<klee::Expr> get_x() const { return x; }
  klee::ref<klee::Expr> get_scale() const { return scale; }
  klee::ref<klee::Expr> get_out() const { return out; }
};

class LnFactory : public ControllerModuleFactory {
public:
  LnFactory() : ControllerModuleFactory(ModuleType::Controller_Ln, "Ln") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
