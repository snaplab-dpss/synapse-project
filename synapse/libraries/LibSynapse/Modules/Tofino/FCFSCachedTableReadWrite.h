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
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;
  symbol_t collision_detected;
  symbol_t index_allocation_failed;

public:
  FCFSCachedTableReadWrite(const BDDNode *_node, DS_ID _cached_table_id, DS_ID _used_table_id, addr_t _obj,
                           const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _read_value, klee::ref<klee::Expr> _write_value,
                           const symbol_t &_map_has_this_key, const symbol_t &_collision_detected, const symbol_t &_index_allocation_failed)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableReadWrite, "FCFSCachedTableReadWrite", _node), cached_table_id(_cached_table_id),
        used_table_id(_used_table_id), obj(_obj), keys(_keys), read_value(_read_value), write_value(_write_value),
        map_has_this_key(_map_has_this_key), collision_detected(_collision_detected), index_allocation_failed(_index_allocation_failed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableReadWrite(node, cached_table_id, used_table_id, obj, keys, read_value, write_value, map_has_this_key,
                                                  collision_detected, index_allocation_failed);
    return cloned;
  }

  DS_ID get_fcfs_cached_table_id() const { return cached_table_id; }
  DS_ID get_used_table_id() const { return used_table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }
  const symbol_t &get_collision_detected() const { return collision_detected; }
  const symbol_t &get_index_allocation_failed() const { return index_allocation_failed; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cached_table_id}; }
};

class FCFSCachedTableReadWriteFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableReadWriteFactory() : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedTableReadWrite, "FCFSCachedTableReadWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse