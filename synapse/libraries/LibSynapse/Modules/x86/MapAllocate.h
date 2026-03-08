#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class MapAllocate : public x86Module {
private:
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> key_size;
  klee::ref<klee::Expr> map_out;

  symbol_t success;

public:
  MapAllocate(const BDDNode *_node, klee::ref<klee::Expr> _capacity, klee::ref<klee::Expr> _key_size, klee::ref<klee::Expr> _map_out,
              symbol_t _success)
      : x86Module(ModuleType::x86_MapAllocate, "MapAllocate", _node), capacity(_capacity), key_size(_key_size), map_out(_map_out), success(_success) {
  }

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new MapAllocate(node, capacity, key_size, map_out, success);
    return cloned;
  }

  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  klee::ref<klee::Expr> get_key_size() const { return key_size; }
  klee::ref<klee::Expr> get_map_out() const { return map_out; }
  symbol_t get_success() const { return success; }
};

class MapAllocateFactory : public x86ModuleFactory {
public:
  MapAllocateFactory() : x86ModuleFactory(ModuleType::x86_MapAllocate, "MapAllocate") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
