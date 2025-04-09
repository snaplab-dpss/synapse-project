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
  LibCore::symbol_t map_has_this_key;

public:
  HHTableRead(const LibBDD::Node *node, DS_ID _hh_table_id, addr_t _obj, klee::ref<klee::Expr> _original_key,
              const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _value, const LibCore::symbol_t &_map_has_this_key)
      : TofinoModule(ModuleType::Tofino_HHTableRead, "HHTableRead", node), hh_table_id(_hh_table_id), obj(_obj), original_key(_original_key),
        keys(_keys), value(_value), map_has_this_key(_map_has_this_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new HHTableRead(node, hh_table_id, obj, original_key, keys, value, map_has_this_key);
    return cloned;
  }

  DS_ID get_hh_table_id() const { return hh_table_id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_original_key() const { return original_key; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  const LibCore::symbol_t &get_hit() const { return map_has_this_key; }

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