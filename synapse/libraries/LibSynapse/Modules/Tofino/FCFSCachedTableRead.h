#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedTableRead : public TofinoModule {
private:
  DS_ID cached_table_id;
  DS_ID used_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value;
  symbol_t map_has_this_key;

public:
  FCFSCachedTableRead(ModuleType _type, const BDDNode *_node, DS_ID _cached_table_id, DS_ID _used_table_id, addr_t _obj, klee::ref<klee::Expr> _key,
                      klee::ref<klee::Expr> _value, const symbol_t &_map_has_this_key)
      : TofinoModule(_type, "FCFSCachedTableRead", _node), cached_table_id(_cached_table_id), used_table_id(_used_table_id), obj(_obj), key(_key),
        value(_value), map_has_this_key(_map_has_this_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableRead(type, node, cached_table_id, used_table_id, obj, key, value, map_has_this_key);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  DS_ID get_used_table_id() const { return used_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cached_table_id}; }
};

class FCFSCachedTableReadFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableReadFactory(const std::string &_instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_FCFSCachedTableRead, _instance_id), "FCFSCachedTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse