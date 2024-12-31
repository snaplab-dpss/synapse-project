#pragma once

#include "tofino_module.h"

namespace tofino {

class FCFSCachedTableDelete : public TofinoModule {
private:
  DS_ID cached_table_id;
  addr_t obj;
  klee::ref<klee::Expr> key;
  symbol_t cached_delete_failed;

public:
  FCFSCachedTableDelete(const Node *node, DS_ID _cached_table_id, addr_t _obj,
                        klee::ref<klee::Expr> _key,
                        const symbol_t &_cached_delete_failed)
      : TofinoModule(ModuleType::Tofino_FCFSCachedTableDelete,
                     "FCFSCachedTableDelete", node),
        cached_table_id(_cached_table_id), obj(_obj), key(_key),
        cached_delete_failed(_cached_delete_failed) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep,
                                  const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new FCFSCachedTableDelete(node, cached_table_id, obj, key,
                                               cached_delete_failed);
    return cloned;
  }

  DS_ID get_cached_table_id() const { return cached_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_key() const { return key; }
  const symbol_t &get_cached_delete_failed() const {
    return cached_delete_failed;
  }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override {
    return {cached_table_id};
  }
};

class FCFSCachedTableDeleteGenerator : public TofinoModuleGenerator {
public:
  FCFSCachedTableDeleteGenerator()
      : TofinoModuleGenerator(ModuleType::Tofino_FCFSCachedTableDelete,
                              "FCFSCachedTableDelete") {}

protected:
  virtual std::optional<spec_impl_t>
  speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep,
                                           const Node *node) const override;
};

} // namespace tofino
