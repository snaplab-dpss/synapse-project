#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

using LibBDD::Call;
using LibBDD::call_t;

class IntegerAllocatorRejuvenate : public TofinoModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> time;

public:
  IntegerAllocatorRejuvenate(const std::string &_instance_id, const BDDNode *_node, addr_t _dchain_addr, klee::ref<klee::Expr> _index,
                             klee::ref<klee::Expr> _time)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_IntegerAllocatorRejuvenate, _instance_id), "IntegerAllocatorRejuvenate", _node),
        dchain_addr(_dchain_addr), index(_index), time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new IntegerAllocatorRejuvenate(get_type().instance_id, node, dchain_addr, index, time);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_time() const { return time; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    // FIXME: Missing the Integer Allocator data structure.
    // return {table_id};
    return {};
  }
};

class IntegerAllocatorRejuvenateFactory : public TofinoModuleFactory {
public:
  IntegerAllocatorRejuvenateFactory(const std::string &_instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_IntegerAllocatorRejuvenate, _instance_id), "IntegerAllocatorRejuvenate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse