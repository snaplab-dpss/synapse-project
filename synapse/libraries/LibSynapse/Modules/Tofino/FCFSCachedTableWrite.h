#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class FCFSCachedTableWrite : public TofinoModule {
private:
  DS_ID cached_table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  symbol_t cache_write_success;

public:
  FCFSCachedTableWrite(const BDDNode *_node, DS_ID _cached_table_id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
                       const symbol_t &_cache_write_success)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableWrite, "FCFSCachedTableWrite", _node), cached_table_id(_cached_table_id), obj(_obj),
        keys(_keys), cache_write_success(_cache_write_success) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableWrite(node, cached_table_id, obj, keys, cache_write_success);
    return cloned;
  }

  DS_ID get_fcfs_cached_table_id() const { return cached_table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const symbol_t &get_cache_write_success() const { return cache_write_success; }

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