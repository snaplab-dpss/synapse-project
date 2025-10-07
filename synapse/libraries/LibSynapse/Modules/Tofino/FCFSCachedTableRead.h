#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedTableRead : public TofinoModule {
private:
  DS_ID cached_table_id;
  DS_ID used_table_id;
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  symbol_t map_has_this_key;

public:
  FCFSCachedTableRead(const BDDNode *_node, DS_ID _cached_table_id, DS_ID _used_table_id, addr_t _obj, klee::ref<klee::Expr> _original_key,
                      const std::vector<klee::ref<klee::Expr>> &_keys, const symbol_t &_map_has_this_key)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableRead, "FCFSCachedTableRead", _node), cached_table_id(_cached_table_id),
        used_table_id(_used_table_id), obj(_obj), original_key(_original_key), keys(_keys), map_has_this_key(_map_has_this_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableRead(node, cached_table_id, used_table_id, obj, original_key, keys, map_has_this_key);
    return cloned;
  }

  DS_ID get_fcfs_cached_table_id() const { return cached_table_id; }
  DS_ID get_used_table_id() const { return used_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_original_key() const { return original_key; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cached_table_id}; }
};

class FCFSCachedTableReadFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableReadFactory() : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedTableRead, "FCFSCachedTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse