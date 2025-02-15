#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class IntegerAllocatorAllocate : public ControllerModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index_range;

public:
  IntegerAllocatorAllocate(const LibBDD::Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _index_range)
      : ControllerModule(ModuleType::Controller_IntegerAllocatorAllocate, "IntegerAllocatorAllocate", node), dchain_addr(_dchain_addr),
        index_range(_index_range) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new IntegerAllocatorAllocate(node, dchain_addr, index_range);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index_range() const { return index_range; }
};

class IntegerAllocatorAllocateFactory : public ControllerModuleFactory {
public:
  IntegerAllocatorAllocateFactory()
      : ControllerModuleFactory(ModuleType::Controller_IntegerAllocatorAllocate, "IntegerAllocatorAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Controller
} // namespace LibSynapse