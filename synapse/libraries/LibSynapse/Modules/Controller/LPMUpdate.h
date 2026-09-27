#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class LPMUpdate : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> prefix;
  klee::ref<klee::Expr> prefixlen;
  klee::ref<klee::Expr> value;

public:
  LPMUpdate(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _prefix, klee::ref<klee::Expr> _prefixlen, klee::ref<klee::Expr> _value)
      : ControllerModule(ModuleType::Controller_LPMUpdate, "LPMUpdate", _node), obj(_obj), prefix(_prefix), prefixlen(_prefixlen), value(_value) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new LPMUpdate(node, obj, prefix, prefixlen, value);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_prefix() const { return prefix; }
  klee::ref<klee::Expr> get_prefixlen() const { return prefixlen; }
  klee::ref<klee::Expr> get_value() const { return value; }
};

class LPMUpdateFactory : public ControllerModuleFactory {
public:
  LPMUpdateFactory() : ControllerModuleFactory(ModuleType::Controller_LPMUpdate, "LPMUpdate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
