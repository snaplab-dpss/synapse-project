#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// Controller-side (CPU) `rotate_left`, evaluated with libnf's implementation (embedded in
// libsycon). Stateless: computes the result from its inputs and binds it for downstream use.
class RotateLeft : public ControllerModule {
private:
  klee::ref<klee::Expr> x;
  klee::ref<klee::Expr> n;
  klee::ref<klee::Expr> out;

public:
  RotateLeft(const BDDNode *_node, klee::ref<klee::Expr> _x, klee::ref<klee::Expr> _n, klee::ref<klee::Expr> _out)
      : ControllerModule(ModuleType::Controller_RotateLeft, "RotateLeft", _node), x(_x), n(_n), out(_out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new RotateLeft(node, x, n, out); }

  klee::ref<klee::Expr> get_x() const { return x; }
  klee::ref<klee::Expr> get_n() const { return n; }
  klee::ref<klee::Expr> get_out() const { return out; }
};

class RotateLeftFactory : public ControllerModuleFactory {
public:
  RotateLeftFactory() : ControllerModuleFactory(ModuleType::Controller_RotateLeft, "RotateLeft") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
