#pragma once

#include <LibSynapse/Modules/x86/x86Module.h>

namespace LibSynapse {
namespace x86 {

class MapGet : public x86Module {
private:
  addr_t map_addr;
  addr_t key_addr;
  klee::ref<klee::Expr> key;
  klee::ref<klee::Expr> value_out;
  klee::ref<klee::Expr> success;
  symbol_t map_has_this_key;

public:
  MapGet(const std::string &_instance_id, const BDDNode *_node, addr_t _map_addr, addr_t _key_addr, klee::ref<klee::Expr> _key,
         klee::ref<klee::Expr> _value_out, klee::ref<klee::Expr> _success, const symbol_t &_map_has_this_key)
      : x86Module(ModuleType(ModuleCategory::x86_MapGet, _instance_id), "MapGet", _node), map_addr(_map_addr), key_addr(_key_addr), key(_key),
        value_out(_value_out), success(_success), map_has_this_key(_map_has_this_key) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new MapGet(get_type().instance_id, node, map_addr, key_addr, key, value_out, success, map_has_this_key);
    return cloned;
  }

  addr_t get_map_addr() const { return map_addr; }
  addr_t get_key_addr() const { return key_addr; }
  klee::ref<klee::Expr> get_key() const { return key; }
  klee::ref<klee::Expr> get_value_out() const { return value_out; }
  klee::ref<klee::Expr> get_success() const { return success; }
  const symbol_t &get_map_has_this_key() const { return map_has_this_key; }
};

class MapGetFactory : public x86ModuleFactory {
public:
  MapGetFactory(const std::string &_instance_id) : x86ModuleFactory(ModuleType(ModuleCategory::x86_MapGet, _instance_id), "MapGet") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const BDDNode *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const BDDNode *node, SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const BDD *bdd, const Context &ctx, const BDDNode *node) const override;
};

} // namespace x86
} // namespace LibSynapse