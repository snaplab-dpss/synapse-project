#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class LPMAllocate : public x86Module {
private:
  klee::ref<klee::Expr> lpm_out;
  symbol_t success;

public:
  LPMAllocate(const std::string &_instance_id, const BDDNode *_node, klee::ref<klee::Expr> _lpm_out, symbol_t _success)
      : x86Module(ModuleType(ModuleCategory::x86_LPMAllocate, _instance_id), "LPMAllocate", _node), lpm_out(_lpm_out), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new LPMAllocate(get_type().instance_id, node, lpm_out, success);
    return cloned;
  }

  klee::ref<klee::Expr> get_lpm_out() const { return lpm_out; }
  symbol_t get_success() const { return success; }
};

class LPMAllocateFactory : public x86ModuleFactory {
public:
  LPMAllocateFactory(const std::string &_instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_LPMAllocate, _instance_id), "LPMAllocate") {}

private:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse