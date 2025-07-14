#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class Recirculate : public TofinoModule {
private:
  Symbols symbols;
  u16 recirc_port;
  u32 code_path;

public:
  Recirculate(const BDDNode *_node, Symbols _symbols, u16 _recirc_port, u32 _code_path)
      : TofinoModule(ModuleType::Tofino_Recirculate, "Recirculate", _node), symbols(_symbols), recirc_port(_recirc_port), code_path(_code_path) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    Recirculate *cloned = new Recirculate(node, symbols, recirc_port, code_path);
    return cloned;
  }

  const Symbols &get_symbols() const { return symbols; }
  u16 get_recirc_port() const { return recirc_port; }
  u32 get_code_path() const { return code_path; }
};

class RecirculateFactory : public TofinoModuleFactory {
public:
  RecirculateFactory() : TofinoModuleFactory(ModuleType::Tofino_Recirculate, "Recirculate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse