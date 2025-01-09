#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class IntegerAllocatorRejuvenate : public TofinoModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> time;

public:
  IntegerAllocatorRejuvenate(const Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _time)
      : TofinoModule(ModuleType::Tofino_IntegerAllocatorRejuvenate, "IntegerAllocatorRejuvenate", node), dchain_addr(_dchain_addr),
        index(_index), time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new IntegerAllocatorRejuvenate(node, dchain_addr, index, time);
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
  IntegerAllocatorRejuvenateFactory() : TofinoModuleFactory(ModuleType::Tofino_IntegerAllocatorRejuvenate, "IntegerAllocatorRejuvenate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse