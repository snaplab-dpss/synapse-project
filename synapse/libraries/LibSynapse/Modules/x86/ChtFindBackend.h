#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class ChtFindBackend : public x86Module {
private:
  addr_t cht_addr;
  addr_t backends_addr;
  klee::ref<klee::Expr> hash;
  klee::ref<klee::Expr> height;
  klee::ref<klee::Expr> capacity;
  klee::ref<klee::Expr> backend;
  symbol_t found;

public:
  ChtFindBackend(const InstanceId _instance_id, const BDDNode *_node, addr_t _cht_addr, addr_t _backends_addr, klee::ref<klee::Expr> _hash,
                 klee::ref<klee::Expr> _height, klee::ref<klee::Expr> _capacity, klee::ref<klee::Expr> _backend, const symbol_t &_found)
      : x86Module(ModuleType(ModuleCategory::x86_ChtFindBackend, _instance_id), "ChtFindBackend", _node), cht_addr(_cht_addr),
        backends_addr(_backends_addr), hash(_hash), height(_height), capacity(_capacity), backend(_backend), found(_found) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new ChtFindBackend(get_type().instance_id, node, cht_addr, backends_addr, hash, height, capacity, backend, found);
    return cloned;
  }

  klee::ref<klee::Expr> get_hash() const { return hash; }
  addr_t get_cht_addr() const { return cht_addr; }
  addr_t get_backends_addr() const { return backends_addr; }
  klee::ref<klee::Expr> get_height() const { return height; }
  klee::ref<klee::Expr> get_capacity() const { return capacity; }
  klee::ref<klee::Expr> get_backend() const { return backend; }
  const symbol_t &get_found() const { return found; }
};

class ChtFindBackendFactory : public x86ModuleFactory {
public:
  ChtFindBackendFactory(const InstanceId _instance_id)
      : x86ModuleFactory(ModuleType(ModuleCategory::x86_ChtFindBackend, _instance_id), "ChtFindBackend") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
