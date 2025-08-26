#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedTableReadWrite : public TofinoModule {
private:
  DS_ID cached_table_id;
  DS_ID used_table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  symbol_t map_has_this_key;
  symbol_t cache_hit;

public:
  FCFSCachedTableReadWrite(const BDDNode *_node, DS_ID _cached_table_id, DS_ID _used_table_id, addr_t _obj,
                           const std::vector<klee::ref<klee::Expr>> &_keys, const symbol_t &_map_has_this_key, const symbol_t &_cache_hit)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableReadWrite, "FCFSCachedTableReadWrite", _node), cached_table_id(_cached_table_id),
        used_table_id(_used_table_id), obj(_obj), keys(_keys), map_has_this_key(_map_has_this_key), cache_hit(_cache_hit) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableReadWrite(node, cached_table_id, used_table_id, obj, keys, map_has_this_key, cache_hit);
    return cloned;
  }

  DS_ID get_fcfs_cached_table_id() const { return cached_table_id; }
  DS_ID get_used_table_id() const { return used_table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }
  const symbol_t &get_cache_hit() const { return cache_hit; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cached_table_id}; }
};

class FCFSCachedTableReadWriteFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableReadWriteFactory(const std::string &_instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_FCFSCachedTableReadWrite, _instance_id), "FCFSCachedTableReadWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse
