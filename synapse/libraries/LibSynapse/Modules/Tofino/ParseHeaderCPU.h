#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class ParseHeaderCPU : public TofinoModule {
private:
  symbol_t code_path;

public:
  ParseHeaderCPU(const BDDNode *_node, symbol_t _code_path)
      : TofinoModule(ModuleType::Tofino_ParseHeaderCPU, "ParseHeaderCPU", _node), code_path(_code_path) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ParseHeaderCPU *cloned = new ParseHeaderCPU(node, code_path);
    return cloned;
  }

  symbol_t get_code_path() const { return code_path; }
};

class ParseHeaderCPUFactory : public TofinoModuleFactory {
public:
  ParseHeaderCPUFactory() : TofinoModuleFactory(ModuleType::Tofino_ParseHeaderCPU, "ParseHeaderCPU") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
