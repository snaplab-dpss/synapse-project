#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class DchainAllocateNewIndex : public x86Module {
private:
  addr_t dchain_addr;
  klee::ref<klee::Expr> time;
  klee::ref<klee::Expr> index_out;
  std::optional<LibCore::symbol_t> not_out_of_space;

public:
  DchainAllocateNewIndex(const LibBDD::Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out,
                         const LibCore::symbol_t &_out_of_space)
      : x86Module(ModuleType::x86_DchainAllocateNewIndex, "DchainAllocate", node), dchain_addr(_dchain_addr), time(_time),
        index_out(_index_out), not_out_of_space(_out_of_space) {}

  DchainAllocateNewIndex(const LibBDD::Node *node, addr_t _dchain_addr, klee::ref<klee::Expr> _time, klee::ref<klee::Expr> _index_out)
      : x86Module(ModuleType::x86_DchainAllocateNewIndex, "DchainAllocate", node), dchain_addr(_dchain_addr), time(_time),
        index_out(_index_out), not_out_of_space(std::nullopt) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned;

    if (not_out_of_space.has_value()) {
      cloned = new DchainAllocateNewIndex(node, dchain_addr, time, index_out, *not_out_of_space);
    } else {
      cloned = new DchainAllocateNewIndex(node, dchain_addr, time, index_out);
    }

    return cloned;
  }

  addr_t get_dchain_addr() const { return dchain_addr; }
  klee::ref<klee::Expr> get_time() const { return time; }
  klee::ref<klee::Expr> get_index_out() const { return index_out; }
  std::optional<LibCore::symbol_t> get_not_out_of_space() const { return not_out_of_space; }
};

class DchainAllocateNewIndexFactory : public x86ModuleFactory {
public:
  DchainAllocateNewIndexFactory() : x86ModuleFactory(ModuleType::x86_DchainAllocateNewIndex, "DchainAllocateNewIndex") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace x86
} // namespace LibSynapse