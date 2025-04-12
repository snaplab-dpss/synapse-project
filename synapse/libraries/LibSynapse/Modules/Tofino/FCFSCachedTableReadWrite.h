#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedTableReadWrite : public TofinoModule {
private:
  DS_ID cached_table_id;
  DS_ID used_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> read_value;
  klee::ref<klee::Expr> write_value;
  LibCore::symbol_t map_has_this_key;
  LibCore::symbol_t cache_write_failed;

public:
  FCFSCachedTableReadWrite(const LibBDD::Node *_node, DS_ID _cached_table_id, DS_ID _used_table_id, addr_t _obj, klee::ref<klee::Expr> _key,
                           klee::ref<klee::Expr> _read_value, klee::ref<klee::Expr> _write_value, const LibCore::symbol_t &_map_has_this_key,
                           const LibCore::symbol_t &_cache_write_failed)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableReadWrite, "FCFSCachedTableReadWrite", _node), cached_table_id(_cached_table_id),
        used_table_id(_used_table_id), obj(_obj), key(_key), read_value(_read_value), write_value(_write_value), map_has_this_key(_map_has_this_key),
        cache_write_failed(_cache_write_failed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned =
        new FCFSCachedTableReadWrite(node, cached_table_id, used_table_id, obj, key, read_value, write_value, map_has_this_key, cache_write_failed);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  DS_ID get_used_table_id() const { return used_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_read_value() const { return read_value; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const LibCore::symbol_t &get_map_has_this_key() const { return map_has_this_key; }
  const LibCore::symbol_t &get_cache_write_failed() const { return cache_write_failed; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cached_table_id}; }
};

class FCFSCachedTableReadWriteFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableReadWriteFactory() : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedTableReadWrite, "FCFSCachedTableReadWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse