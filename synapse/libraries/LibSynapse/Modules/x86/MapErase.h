#pragma once

#include "LibCore/Types.h"
#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class MapErase : public x86Module {
private:
  klee::ref<klee::Expr> map_addr;
  klee::ref<klee::Expr> key_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> trash;

public:
  MapErase(const InstanceId _instance_id, const BDDNode *_node, klee::ref<klee::Expr> _map_addr, klee::ref<klee::Expr> _key_addr,
           klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _trash)
      : x86Module(ModuleType(ModuleCategory::x86_MapErase, _instance_id), "MapErase", _node), map_addr(_map_addr), key_addr(_key_addr), key(_key),
        trash(_trash) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new MapErase(get_type().instance_id, node, map_addr, key_addr, key, trash);
    return cloned;
  }

  klee::ref<klee::Expr> get_map_addr() const { return map_addr; }
  klee::ref<klee::Expr> get_key_addr() const { return key_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_trash() const { return trash; }
};

class MapEraseFactory : public x86ModuleFactory {
public:
  MapEraseFactory(const InstanceId _instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_MapErase, _instance_id), "MapErase") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse
