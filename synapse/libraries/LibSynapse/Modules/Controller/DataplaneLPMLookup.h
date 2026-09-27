#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class DataplaneLPMLookup : public ControllerModule {
private:
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  klee::ref<klee::Expr> match;

public:
  DataplaneLPMLookup(const BDDNode *_node, addr_t _obj, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _value, klee::ref<klee::Expr> _match)
      : ControllerModule(ModuleType::Controller_DataplaneLPMLookup, "DataplaneLPMLookup", _node), obj(_obj), key(_key), value(_value), match(_match) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DataplaneLPMLookup(node, obj, key, value, match);
    return cloned;
  }

  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
  klee::ref<klee::Expr> get_match() const { return match; }
};

class DataplaneLPMLookupFactory : public ControllerModuleFactory {
public:
  DataplaneLPMLookupFactory() : ControllerModuleFactory(ModuleType::Controller_DataplaneLPMLookup, "DataplaneLPMLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse
