#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

// Controller-side (CPU) `find_first_set_bit`, evaluated with libnf's implementation
// (embedded in libsycon). Stateless: computes the result from its input and binds it
// for downstream use. Used when the operation is offloaded to the controller.
class FindFirstSetBit : public ControllerModule {
private:
  klee::ref<klee::Expr> x;
  klee::ref<klee::Expr> out;

public:
  FindFirstSetBit(const BDDNode *_node, klee::ref<klee::Expr> _x, klee::ref<klee::Expr> _out)
      : ControllerModule(ModuleType::Controller_FindFirstSetBit, "FindFirstSetBit", _node), x(_x), out(_out) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new FindFirstSetBit(node, x, out); }

  klee::ref<klee::Expr> get_x() const { return x; }
  klee::ref<klee::Expr> get_out() const { return out; }
};

class FindFirstSetBitFactory : public ControllerModuleFactory {
public:
  FindFirstSetBitFactory() : ControllerModuleFactory(ModuleType::Controller_FindFirstSetBit, "FindFirstSetBit") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
