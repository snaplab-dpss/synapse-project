#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class DchainRejuvenateIndex : public x86Module {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> index;
  klee::ref<klee::Expr> time;

public:
  DchainRejuvenateIndex(const LibBDD::Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _index, klee::ref<klee::Expr> _time)
      : x86Module(ModuleType::x86_DchainRejuvenateIndex, "DchainRejuvenate", node), dchain_addr(_dchain_addr), index(_index), time(_time) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new DchainRejuvenateIndex(node, dchain_addr, index, time);
    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_index() const { return index; }
  klee::ref<klee::Expr> get_time() const { return time; }
};

class DchainRejuvenateIndexFactory : public x86ModuleFactory {
public:
  DchainRejuvenateIndexFactory() : x86ModuleFactory(ModuleType::x86_DchainRejuvenateIndex, "DchainRejuvenateIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace x86
} // namespace LibSynapse