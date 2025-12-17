#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class ParseHeaderCPU : public x86Module {
private:
  symbol_t code_path;

public:
  ParseHeaderCPU(const InstanceId _instance_id, const BDDNode *_node, symbol_t _code_path)
      : x86Module(ModuleType(ModuleCategory::x86_ParseHeaderCPU, _instance_id), "ParseHeaderCPU", _node), code_path(_code_path) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const {
    ParseHeaderCPU *cloned = new ParseHeaderCPU(get_type().instance_id, node, code_path);
    return cloned;
  }

  symbol_t get_code_path() const { return code_path; }
};

class ParseHeaderCPUFactory : public x86ModuleFactory {
public:
  ParseHeaderCPUFactory(const InstanceId _instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_ParseHeaderCPU, _instance_id), "ParseHeaderCPU") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
