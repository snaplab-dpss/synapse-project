#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class IntegerAllocatorAllocate : public TofinoModule {
private:
  DS_ID table_id;
  addr_t dchain_addr;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> index_out;
  symbol_t not_out_of_space;

public:
  IntegerAllocatorAllocate(const BDDNode *_node, addr_t _dchain_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out,
                           const symbol_t &_out_of_space)
      : TofinoModule(ModuleType::Tofino_IntegerAllocatorAllocate, "IntegerAllocatorAllocate", _node), dchain_addr(_dchain_addr), time(_time),
        index_out(_index_out), not_out_of_space(_out_of_space) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override { return new IntegerAllocatorAllocate(node, dchain_addr, time, index_out, not_out_of_space); }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  const symbol_t &get_not_out_of_space() const { return not_out_of_space; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    // FIXME: Missing the Integer Allocator data structure.
    // return {table_id};
    return {};
  }
};

class IntegerAllocatorAllocateFactory : public TofinoModuleFactory {
public:
  IntegerAllocatorAllocateFactory() : TofinoModuleFactory(ModuleType::Tofino_IntegerAllocatorAllocate, "IntegerAllocatorAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse