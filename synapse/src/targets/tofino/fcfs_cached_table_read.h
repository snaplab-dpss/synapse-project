#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class FCFSCachedTableRead : public TofinoModule {
private:
  DS_ID cached_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  symbol_t map_has_this_key;

public:
  FCFSCachedTableRead(const Node *node, DS_ID _cached_table_id, addr_t _obj, klee::ref<klee::Expr> _key,
                      klee::ref<klee::Expr> _value, const symbol_t &_map_has_this_key)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableRead, "FCFSCachedTableRead", node),
        cached_table_id(_cached_table_id), obj(_obj), key(_key), value(_value), map_has_this_key(_map_has_this_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableRead(node, cached_table_id, obj, key, value, map_has_this_key);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cached_table_id}; }
};

class FCFSCachedTableReadFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableReadFactory() : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedTableRead, "FCFSCachedTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse