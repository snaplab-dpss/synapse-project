#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// Controller-side (CPU) `min`, evaluated with libnf's implementation (embedded in
// libsycon). Stateless: computes the result from its inputs and binds it for downstream
// use. Used when the operation is offloaded to the controller.
class Min : public ControllerModule {
private:
  klee::ref<klee::Expr> a;
  klee::ref<klee::Expr> b;
  klee::ref<klee::Expr> out;

public:
  Min(const BDDNode *_node, klee::ref<klee::Expr> _a, klee::ref<klee::Expr> _b, klee::ref<klee::Expr> _out)
      : ControllerModule(ModuleType::Controller_Min, "Min", _node), a(_a), b(_b), out(_out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new Min(node, a, b, out); }

  klee::ref<klee::Expr> get_a() const { return a; }
  klee::ref<klee::Expr> get_b() const { return b; }
  klee::ref<klee::Expr> get_out() const { return out; }
};

class MinFactory : public ControllerModuleFactory {
public:
  MinFactory() : ControllerModuleFactory(ModuleType::Controller_Min, "Min") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
