#pragma once

#include <LibSynapse/Modules/Controller/ControllerModule.h>

namespace LibSynapse {
namespace Controller {

class MapErase : public ControllerModule {
private:
  addr_t map_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> trash;

public:
  MapErase(const std::string &_instance_id, const BDDNode *_node, addr_t _map_addr, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _trash)
      : ControllerModule(ModuleType(ModuleCategory::Controller_MapErase, _instance_id), "MapErase", _node), map_addr(_map_addr), key(_key),
        trash(_trash) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new MapErase(get_type().instance_id, node, map_addr, key, trash);
    return cloned;
  }

  addr_t get_map_addr() const { return map_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_trash() const { return trash; }
};

class MapEraseFactory : public ControllerModuleFactory {
public:
  MapEraseFactory(const std::string &_instance_id)
      : ControllerModuleFactory(ModuleType(ModuleCategory::Controller_MapErase, _instance_id), "MapErase") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Controller
} // namespace LibSynapse