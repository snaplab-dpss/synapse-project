#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class TableLookup : public TofinoModule {
private:
  DS_ID table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  std::vector<klee::ref<klee::Expr>> values;
  std::optional<LibCore::symbol_t> hit;

public:
  TableLookup(const LibBDD::Node *node, DS_ID _table_id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
              const std::vector<klee::ref<klee::Expr>> &_values, const std::optional<LibCore::symbol_t> &_hit)
      : TofinoModule(ModuleType::Tofino_TableLookup, "TableLookup", node), table_id(_table_id), obj(_obj), keys(_keys), values(_values),
        hit(_hit) {}

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
  const std::optional<LibCore::symbol_t> &get_hit() const { return hit; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {table_id}; }
};

class TableLookupFactory : public TofinoModuleFactory {
public:
  TableLookupFactory() : TofinoModuleFactory(ModuleType::Tofino_TableLookup, "TableLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
};

} // namespace Tofino
} // namespace LibSynapse