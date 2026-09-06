#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedTableRead : public TofinoModule {
private:
  DS_ID fcfs_ct_id;
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  symbol_t map_has_this_key;
  // The BDD rejuvenates this map's dchain on the found path, so a cache hit must refresh
  // the entry's liveness timestamp (query-and-refresh) instead of only querying it.
  bool refresh;

public:
  FCFSCachedTableRead(const BDDNode *_node, DS_ID _fcfs_ct_id, addr_t _obj, klee::ref<klee::Expr> _original_key,
                      const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _value, const symbol_t &_map_has_this_key, bool _refresh)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableRead, "FCFSCachedTableRead", _node), fcfs_ct_id(_fcfs_ct_id), obj(_obj),
        original_key(_original_key), keys(_keys), value(_value), map_has_this_key(_map_has_this_key), refresh(_refresh) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableRead(node, fcfs_ct_id, obj, original_key, keys, value, map_has_this_key, refresh);
    return cloned;
  }

  DS_ID get_fcfs_ct_id() const { return fcfs_ct_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_original_key() const { return original_key; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }
  bool get_refresh() const { return refresh; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {fcfs_ct_id}; }
};

class FCFSCachedTableReadFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableReadFactory() : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedTableRead, "FCFSCachedTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const speculations_t &speculations) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse