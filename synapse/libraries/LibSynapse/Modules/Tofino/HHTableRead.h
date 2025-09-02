#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class HHTableRead : public TofinoModule {
private:
  DS_ID hh_table_id;
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  symbol_t map_has_this_key;

public:
  HHTableRead(const std::string &_instance_id, const BDDNode *_node, DS_ID _hh_table_id, addr_t _obj, klee::ref<klee::Expr> _original_key,
              const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _value, const symbol_t &_map_has_this_key)
      : TofinoModule(ModuleType(ModuleCategory::Tofino_HHTableRead, _instance_id), "HHTableRead", _node), hh_table_id(_hh_table_id), obj(_obj),
        original_key(_original_key), keys(_keys), value(_value), map_has_this_key(_map_has_this_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new HHTableRead(get_type().instance_id, node, hh_table_id, obj, original_key, keys, value, map_has_this_key);
    return cloned;
  }

  DS_ID get_hh_table_id() const { return hh_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_original_key() const { return original_key; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const symbol_t &get_hit() const { return map_has_this_key; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {hh_table_id}; }
};

class HHTableReadFactory : public TofinoModuleFactory {
public:
  HHTableReadFactory(const std::string &_instance_id)
      : TofinoModuleFactory(ModuleType(ModuleCategory::Tofino_HHTableRead, _instance_id), "HHTableRead") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse