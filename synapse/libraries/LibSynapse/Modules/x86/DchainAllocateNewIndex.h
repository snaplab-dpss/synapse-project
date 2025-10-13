#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class DchainAllocateNewIndex : public x86Module {
private:
  klee::ref<klee::Expr> dchain_addr;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> index_out;
  std::optional<symbol_t> not_out_of_space;

public:
  DchainAllocateNewIndex(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _dchain_addr, klee::ref<klee::Expr> _time,
                         klee::ref<klee::Expr> _index_out, const symbol_t &_out_of_space)
      : x86Module(ModuleType(ModuleCategory::x86_DchainAllocateNewIndex, _instance_id), "DchainAllocate", _node), dchain_addr(_dchain_addr),
        time(_time), index_out(_index_out), not_out_of_space(_out_of_space) {}

  DchainAllocateNewIndex(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _dchain_addr, klee::ref<klee::Expr> _time,
                         klee::ref<klee::Expr> _index_out)
      : x86Module(ModuleType(ModuleCategory::x86_DchainAllocateNewIndex, _instance_id), "DchainAllocate", _node), dchain_addr(_dchain_addr),
        time(_time), index_out(_index_out), not_out_of_space(std::nullopt) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned;

    if (not_out_of_space.has_value()) {
      cloned = new DchainAllocateNewIndex(get_type().instance_id, node, dchain_addr, time, index_out, *not_out_of_space);
    } else {
      cloned = new DchainAllocateNewIndex(get_type().instance_id, node, dchain_addr, time, index_out);
    }

    return cloned;
  }

  klee::ref<klee::Expr> get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  std::optional<symbol_t> get_not_out_of_space() const { return not_out_of_space; }
};

class DchainAllocateNewIndexFactory : public x86ModuleFactory {
public:
  DchainAllocateNewIndexFactory(const InstanceId _instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_DchainAllocateNewIndex, _instance_id), "DchainAllocateNewIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse