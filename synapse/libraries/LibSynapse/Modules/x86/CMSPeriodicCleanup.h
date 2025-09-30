#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class CMSPeriodicCleanup : public x86Module {
private:
  klee::ref<klee::Expr> cms_addr;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> cleanup_success;

public:
  CMSPeriodicCleanup(const std::string &_instance_id, const BDDNode *_node, klee::ref<klee::Expr> _cms_addr, klee::ref<klee::Expr> _time,
                     klee::ref<klee::Expr> _cleanup_success)
      : x86Module(ModuleType(ModuleCategory::x86_CMSPeriodicCleanup, _instance_id), "CMSPeriodicCleanup", _node), cms_addr(_cms_addr), time(_time),
        cleanup_success(_cleanup_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new CMSPeriodicCleanup(get_type().instance_id, node, cms_addr, time, cleanup_success);
    return cloned;
  }

  klee::ref<klee::Expr> get_cms_addr() const { return cms_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_cleanup_success() const { return cleanup_success; }
};

class CMSPeriodicCleanupFactory : public x86ModuleFactory {
public:
  CMSPeriodicCleanupFactory(const std::string &_instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_CMSPeriodicCleanup, _instance_id), "CMSPeriodicCleanup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
