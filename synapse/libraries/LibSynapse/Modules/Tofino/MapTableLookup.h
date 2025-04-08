#pragma once

#include <LibSynapse/Modules/Tofino/TofinoModule.h>

namespace LibSynapse {
namespace Tofino {

class MapTableLookup : public TofinoModule {
private:
  DS_ID id;
  addr_t obj;
  klee::ref<klee::Expr> original_key;
  std::vector<klee::ref<klee::Expr>> keys;
  klee::ref<klee::Expr> value;
  std::optional<LibCore::symbol_t> hit;

public:
  MapTableLookup(const LibBDD::Node *node, DS_ID _id, addr_t _obj, klee::ref<klee::Expr> _original_key,
                 const std::vector<klee::ref<klee::Expr>> &_keys, klee::ref<klee::Expr> _value, std::optional<LibCore::symbol_t> _hit)
      : TofinoModule(ModuleType::Tofino_MapTableLookup, "MapTableLookup", node), id(_id), obj(_obj), original_key(_original_key), keys(_keys),
        value(_value), hit(_hit) {}

  virtual EPVisitor::Action visit(EPVisitor &visitor, const EP *ep, const EPNode *ep_node) const override { return visitor.visit(ep, ep_node, this); }

  virtual Module *clone() const override {
    Module *cloned = new MapTableLookup(node, id, obj, original_key, keys, value, hit);
    return cloned;
  }

  DS_ID get_id() const { return id; }
  addr_t get_obj() const { return obj; }
  klee::ref<klee::Expr> get_original_key() const { return original_key; }
  const std::vector<klee::ref<klee::Expr>> &get_keys() const { return keys; }
  klee::ref<klee::Expr> get_value() const { return value; }
  std::optional<LibCore::symbol_t> get_hit() const { return hit; }

  virtual std::unordered_set<DS_ID> get_generated_ds() const override { return {id}; }
};

class MapTableLookupFactory : public TofinoModuleFactory {
public:
  MapTableLookupFactory() : TofinoModuleFactory(ModuleType::Tofino_MapTableLookup, "MapTableLookup") {}

protected:
  virtual std::optional<spec_impl_t> speculate(const EP *ep, const LibBDD::Node *node, const Context &ctx) const override;
  virtual std::vector<impl_t> process_node(const EP *ep, const LibBDD::Node *node, LibCore::SymbolManager *symbol_manager) const override;
  virtual std::unique_ptr<Module> create(const LibBDD::BDD *bdd, const Context &ctx, const LibBDD::Node *node) const override;
};

} // namespace Tofino
} // namespace LibSynapse