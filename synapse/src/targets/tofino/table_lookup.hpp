#pragma once

#include "tofino_module.hpp"

namespace synapse {
namespace tofino {

class TableLookup : public TofinoModule {
private:
  DS_ID table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<symbol_t> hit;

public:
  TableLookup(const Node *node, DS_ID _table_id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
              const std::vector<klee::ref<klee::Expr>> &_values, const std::optional<symbol_t> &_hit)
      : TofinoModule(ModuleType::Tofino_TableLookup, "TableLookup", node), table_id(_table_id), obj(_obj), keys(_keys),
        values(_values), hit(_hit) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new TableLookup(node, table_id, obj, keys, values, hit);
    return cloned;
  }

  DS_ID get_table_id() const { return table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  const std::vector<klee::ref<klee::Expr>> &get_values() const { return values; }
  const std::optional<symbol_t> &get_hit() const { return hit; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {table_id}; }
};

class TableLookupFactory : public TofinoModuleFactory {
public:
  TableLookupFactory() : TofinoModuleFactory(ModuleType::Tofino_TableLookup, "TableLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse