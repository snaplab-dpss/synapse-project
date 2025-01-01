#pragma once

#include "tofino_module.h"

namespace tofino {

class MapRegisterDelete : public TofinoModule {
private:
  DS_ID map_register_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  symbol_t cached_delete_failed;

public:
  MapRegisterDelete(const Node *node, DS_ID _map_register_id, addr_t _obj,
                    klee::ref<klee::Expr> _key)
      : TofinoModule(ModuleType::Tofino_MapRegisterDelete, "MapRegisterDelete", node),
        map_register_id(_map_register_id), obj(_obj), key(_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new MapRegisterDelete(node, map_register_id, obj, key);
    return cloned;
  }

  DS_ID get_map_register_id() const { return map_register_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {map_register_id};
  }
};

class MapRegisterDeleteFactory : public TofinoModuleFactory {
public:
  MapRegisterDeleteFactory()
      : TofinoModuleFactory(ModuleType::Tofino_MapRegisterDelete, "MapRegisterDelete") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node) const override;
};

} // namespace tofino
