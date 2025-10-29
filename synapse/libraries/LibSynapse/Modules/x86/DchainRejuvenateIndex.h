#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class DchainRejuvenateIndex : public x86Module {
private:
  klee::ref<klee::Expr> dchain_addr;
  klee::ref<klee::Expr> dchain_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> time;

public:
  DchainRejuvenateIndex(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _dchain_addr, klee::ref<klee::Expr> _index,
                        klee::ref<klee::Expr> _time)
      : x86Module(ModuleType(ModuleCategory::x86_DchainRejuvenateIndex, _instance_id), "DchainRejuvenate", _node), dchain_addr(_dchain_addr),
        index(_index), time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DchainRejuvenateIndex(get_type().instance_id, node, dchain_addr, index, time);
    Module *cloned = new DchainRejuvenateIndex(get_type().instance_id, node, dchain_addr, index, time);
    return cloned;
  }

  klee::ref<klee::Expr> get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_time() const { return time; }
};

class DchainRejuvenateIndexFactory : public x86ModuleFactory {
public:
  DchainRejuvenateIndexFactory(const InstanceId _instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_DchainRejuvenateIndex, _instance_id), "DchainRejuvenateIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse