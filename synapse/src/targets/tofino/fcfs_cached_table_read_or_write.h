#pragma once

#include "tofino_module.h"

namespace synapse {
namespace tofino {

class FCFSCachedTableReadOrWrite : public TofinoModule {
private:
  DS_ID cached_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  symbol_t map_has_this_key;
  symbol_t cache_write_failed;

public:
  FCFSCachedTableReadOrWrite(const Node *node, DS_ID _cached_table_id, addr_t _obj,
                             klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _read_value,
                             klee::ref<klee::Expr> _write_value, const symbol_t &_map_has_this_key,
                             const symbol_t &_cache_write_failed)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableReadOrWrite, "FCFSCachedTableReadOrWrite",
                     node),
        cached_table_id(_cached_table_id), obj(_obj), key(_key), read_value(_read_value),
        write_value(_write_value), map_has_this_key(_map_has_this_key),
        cache_write_failed(_cache_write_failed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned =
        new FCFSCachedTableReadOrWrite(node, cached_table_id, obj, key, read_value, write_value,
                                       map_has_this_key, cache_write_failed);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }
  const symbol_t &get_cache_write_failed() const { return cache_write_failed; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cached_table_id}; }
};

class FCFSCachedTableReadOrWriteFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableReadOrWriteFactory()
      : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedTableReadOrWrite,
                            "FCFSCachedTableReadOrWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node,
                                               const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse