#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedTableWrite : public TofinoModule {
private:
  DS_ID cached_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> write_value;
  symbol_t cache_write_failed;

public:
  FCFSCachedTableWrite(const BDDNode *_node, DS_ID _cached_table_id, addr_t _obj, klee::ref<klee::Expr> _key, klee::ref<klee::Expr> _write_value,
                       const symbol_t &_cache_write_failed)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableWrite, "FCFSCachedTableWrite", _node), cached_table_id(_cached_table_id), obj(_obj), key(_key),
        write_value(_write_value), cache_write_failed(_cache_write_failed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableWrite(node, cached_table_id, obj, key, write_value, cache_write_failed);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_write_value() const { return write_value; }
  const symbol_t &get_cache_write_failed() const { return cache_write_failed; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {cached_table_id}; }
};

class FCFSCachedTableWriteFactory : public TofinoModuleFactory {
public:
  FCFSCachedTableWriteFactory() : TofinoModuleFactory(ModuleType::Tofino_FCFSCachedTableWrite, "FCFSCachedTableWrite") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse