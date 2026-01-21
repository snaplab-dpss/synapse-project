#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedSetReadInsert : public TofinoModule {
private:
  DS_ID fcfs_cs_id;
  DS_ID used_table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;
  symbol_t cached_insert_success;

public:
  FCFSCachedSetReadInsert(const BDDNode *_node, DS_ID _fcfs_cs_id, DS_ID _used_table_id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
                          klee::ref<klee::Expr> _read_value, klee::ref<klee::Expr> _write_value, const symbol_t &_map_has_this_key,
                          const symbol_t &_cached_insert_success)
      : TofinoModule(ModuleType::Tofino_FCFSCachedSetReadInsert, "FCFSCachedSetReadInsert", _node), fcfs_cs_id(_fcfs_cs_id),
        used_table_id(_used_table_id), obj(_obj), keys(_keys), read_value(_read_value), write_value(_write_value),
        map_has_this_key(_map_has_this_key), cached_insert_success(_cached_insert_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned =
        new FCFSCachedSetReadInsert(node, fcfs_cs_id, used_table_id, obj, keys, read_value, write_value, map_has_this_key, cached_insert_success);
    return cloned;
  }

  DS_ID get_fcfs_cs_id() const { return fcfs_cs_id; }
  DS_ID get_used_table_id() const { return used_table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }
  const symbol_t &get_cached_insert_success() const { return cached_insert_success; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {fcfs_cs_id}; }
};

class FCFSCachedSetReadInsertFactory : public TofinoModuleFactory {
public:
  FCFSCachedSetReadInsertFactory() : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedSetReadInsert, "FCFSCachedSetReadInsert") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse