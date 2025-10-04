#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class Recirculate : public TofinoModule {
private:
  Symbols symbols;
  u32 code_path;

public:
  Recirculate(const std::string &_instance_id, const BDDNode *_node, Symbols _symbols, u32 _code_path)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_Recirculate, _instance_id), "Recirculate", _node), symbols(_symbols), code_path(_code_path) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    Recirculate *cloned = new Recirculate(get_type().instance_id, node, symbols, code_path);
    return cloned;
  }

  const Symbols &get_symbols() const { return symbols; }
  u32 get_code_path() const { return code_path; }
};

class RecirculateFactory : public TofinoModuleFactory {
public:
  RecirculateFactory(const std::string &_instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_Recirculate, _instance_id), "Recirculate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
