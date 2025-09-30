#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class DchainAllocate : public x86Module {
private:
  klee::ref<klee::Expr> index_range;
  klee::ref<klee::Expr> chain_out;
  symbol_t success;

public:
  DchainAllocate(const std::string &_instance_id, const BDDNode *_node, klee::ref<klee::Expr> _index_range, klee::ref<klee::Expr> _chain_out,
                 symbol_t _success)
      : x86Module(ModuleType(ModuleCategory::x86_DchainAllocate, _instance_id), "DchainAllocate", _node), index_range(_index_range),
        chain_out(_chain_out), success(_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new DchainAllocate(get_type().instance_id, node, index_range, chain_out, success);
    return cloned;
  }
  klee::ref<klee::Expr> get_index_range() const { return index_range; }
  klee::ref<klee::Expr> get_chain_out() const { return chain_out; }
  symbol_t get_success() const { return success; }
};

class DchainAllocateFactory : public x86ModuleFactory {
public:
  DchainAllocateFactory(const std::string &_instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_DchainAllocate, _instance_id), "DchainAllocate") {}

  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
