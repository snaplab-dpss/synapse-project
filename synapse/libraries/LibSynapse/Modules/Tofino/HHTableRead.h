#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class HHTableRead : public TofinoModule {
private:
  DS_ID hh_table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  LibCore::symbol_t map_has_this_key;
  LibCore::symbol_t min_estimate;

public:
  HHTableRead(const LibBDD::Node *node, DS_ID _hh_table_id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _value, const LibCore::symbol_t &_map_has_this_key, const LibCore::symbol_t &_min_estimate)
      : TofinoModule(ModuleType::Tofino_HHTableRead, "HHTableRead", node), hh_table_id(_hh_table_id), obj(_obj), keys(_keys), value(_value),
        map_has_this_key(_map_has_this_key), min_estimate(_min_estimate) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override {
    return visitor.visit(ep, ep_node, this);
  }

  virtual Module *clone() const override {
    Module *cloned = new HHTableRead(node, hh_table_id, obj, keys, value, map_has_this_key, min_estimate);
    return cloned;
  }

  DS_ID get_hh_table_id() const { return hh_table_id; }
  addr_t get_obj() const { return obj; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const LibCore::symbol_t &get_hit() const { return map_has_this_key; }
  const LibCore::symbol_t &get_min_estimate() const { return min_estimate; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {hh_table_id}; }
};

class HHTableReadFactory : public TofinoModuleFactory {
public:
  HHTableReadFactory() : TofinoModuleFactory(ModuleType::Tofino_HHTableRead, "HHTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse