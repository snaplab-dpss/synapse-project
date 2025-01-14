#pragma once

#include "tofino_module.hpp"

namespace synapse {
namespace tofino {

class HHTableRead : public TofinoModule {
private:
  DS_ID hh_table_id;
  addr_t obj;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  symbol_t map_has_this_key;
  symbol_t min_estimate;

public:
  HHTableRead(const Node *node, DS_ID _hh_table_id, addr_t _obj, const std::vector<klee::ref<klee::Expr>> &_keys,
              klee::ref<klee::Expr> _value, const symbol_t &_map_has_this_key, const symbol_t &_min_estimate)
      : TofinoModule(ModuleType::Tofino_HHTableRead, "HHTableRead", node), hh_table_id(_hh_table_id), obj(_obj),
        keys(_keys), value(_value), map_has_this_key(_map_has_this_key), min_estimate(_min_estimate) {}

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
  const symbol_t &get_hit() const { return map_has_this_key; }
  const symbol_t &get_min_estimate() const { return min_estimate; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {hh_table_id}; }
};

class HHTableReadFactory : public TofinoModuleFactory {
public:
  HHTableReadFactory() : TofinoModuleFactory(ModuleType::Tofino_HHTableRead, "HHTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const Node *node, const Context &ctx) const override;

  virtual std::vector<impl_t> process_node(const EP *ep, const Node *node,
                                           SymbolManager *symbol_manager) const override;
};

} // namespace tofino
} // namespace synapse