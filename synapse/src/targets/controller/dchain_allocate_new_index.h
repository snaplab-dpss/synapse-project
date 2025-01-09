#pragma once

#include "controller_module.h"

namespace synapse {
namespace ctrl {

class DchainAllocateNewIndex : public ControllerModule {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> index_out;
  std::optional<symbol_t> out_of_space;

public:
  DchainAllocateNewIndex(const Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out,
                         const symbol_t &_out_of_space)
      : ControllerModule(ModuleType::Controller_DchainAllocateNewIndex, "DchainAllocate", node), dchain_addr(_dchain_addr), time(_time),
        index_out(_index_out), out_of_space(_out_of_space) {}

  DchainAllocateNewIndex(const Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out)
      : ControllerModule(ModuleType::Controller_DchainAllocateNewIndex, "DchainAllocate", node), dchain_addr(_dchain_addr), time(_time),
        index_out(_index_out), out_of_space(std::nullopt) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned;

    if (out_of_space.has_value()) {
      cloned = new DchainAllocateNewIndex(node, dchain_addr, time, index_out, *out_of_space);
    } else {
      cloned = new DchainAllocateNewIndex(node, dchain_addr, time, index_out);
    }

    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }

  const std::optional<symbol_t> &get_out_of_space() const { return out_of_space; }
};

class DchainAllocateNewIndexFactory : public ControllerModuleFactory {
public:
  DchainAllocateNewIndexFactory() : ControllerModuleFactory(ModuleType::Controller_DchainAllocateNewIndex, "DchainAllocateNewIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node, SymbolManager *symbol_manager) const override;
};

} // namespace ctrl
} // namespace synapse